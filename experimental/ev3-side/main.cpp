#include "ev3dev.h"

#include <stdio.h>
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <unistd.h> //close
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <chrono>
#include <termios.h> // tcgetattr  tcsetattr
#include <fcntl.h>
#include <signal.h> //sigaction

using namespace ev3dev;
using namespace std;

#ifdef EV3
	#define MOTOR_A OUTPUT_A
	#define MOTOR_B OUTPUT_B
	#define MOTOR_C OUTPUT_C
	#define MOTOR_D OUTPUT_D
	void beep() { ev3dev::sound::beep();}
#endif

#ifdef BRICKPI
	#define MOTOR_A "ttyAMA0:outA"
	#define MOTOR_B "ttyAMA0:outB"
	#define MOTOR_C "ttyAMA0:outC"
	#define MOTOR_D "ttyAMA0:outD"
	void beep() { printf("BEEP!\n");}
#endif

// NETWORK CONSTANTS
const int BUFFER_SIZE=80;
const int TIMEOUT_MS=500;
const char *HOST = "192.168.0.20";
const int PORT=5679;

// LASER CONSTANTS
const char *LASER_TTY="/dev/tty_in1";
const char *LASER_PORT_MODE="/sys/class/lego-port/port0/mode";
const char *LASER_UART_MODE="other-uart";

// GYRO CONSTANTS
const char  *GYRO_I2C_BUS_NEW_DEVICE="/sys/bus/i2c/devices/i2c-6/new_device";
const char *GYRO_DEVICE="mi-xg1300l 0x01";
const char *GYRO_PORT="in4:i2c1";
const char *GYRO_POLL_MS="/sys/class/lego-sensor/sensor0/poll_ms";
const char *GYRO_POLL_MS_SETTING="0"; //DISABLE POLLING!
const char *GYRO_DIRECT="/sys/class/lego-sensor/sensor0/direct";

const int LASER_FRAMES_PER_PACKET=15;

struct laser_reading
{
	unsigned int distance : 14;
	unsigned int strength_warning : 1;
	unsigned int invalid_data : 1;	
	uint16_t signal_strength;
} __attribute__((packed));

struct laser_frame
{
	uint8_t start;
	uint8_t index;
	uint16_t speed;
	laser_reading readings[4];
	uint16_t checksum;
	
} __attribute__((packed));;

struct robot_frame
{
	uint16_t laser_angle;
	uint16_t laser_speed;
	uint16_t elapsed_ms; //elapsed_ms/100 is in milliseconds, e.g. 100000 is 1000 ms
	laser_reading laser_readings[4*LASER_FRAMES_PER_PACKET];
	int16_t angle_start;
	int16_t angle_stop;
	int left_start;
	int right_start;
	int left_end;
	int right_end;
} __attribute__((packed));


volatile sig_atomic_t g_finish_program=0;

void InitLaserMotor(motor& m, int duty_cycle);
void InitDriveMotor(large_motor& m);
int InitGyro(sensor& gyro);
int16_t ReadGyroAngle(int gyro_direct_fd);

int InitSocketUDP(int port);
void InitDestination(sockaddr_in &si_dest, const char *host, int port);

void Stop(motor &left, motor& right, motor &head);

void Sleep(int ms);
void Die(const char *s);

void ProcessArguments(int argc, char **argv, int &duty_cycle);
void SetDeviceAttribute(const char *device, const char *attribute, bool fail_silently=false);

int InitLaser(const char* device, termios &old_io);
void CloseLaser(int fd, termios &old_io);
void SynchronizeLaser(int fd);
uint16_t Checksum(const uint8_t data[20]);
bool ReadLaser(int fd, laser_frame frame[LASER_FRAMES_PER_PACKET]);
void SendReadings(int fd, const sockaddr_in &dst, const laser_reading readings[360]);
void ScanLoop(int laser_fd, int gyro_direct_fd, const large_motor& left, const large_motor &right, int dest_fd, const sockaddr_in &dst);
void SendReadingsFrame(int fd, const sockaddr_in &dst, const robot_frame &frame);
void Finish(int signal);
void RegisterSignals();

int main(int argc, char **argv)
{
	int laser_fd, dest_fd, duty_cycle, gyro_direct_fd;
	sockaddr_in si_dest;
	termios old_io;
	
	SetDeviceAttribute(GYRO_I2C_BUS_NEW_DEVICE, GYRO_DEVICE, true); 
	
	motor laser_motor(MOTOR_B);
	large_motor left(MOTOR_D);	
	large_motor right(MOTOR_A);

	sensor gyro(GYRO_PORT);
		
	ProcessArguments(argc,argv, duty_cycle);

	RegisterSignals();

	dest_fd=InitSocketUDP(PORT);
	InitDestination(si_dest, HOST, PORT);
	
	InitDriveMotor(left);
	InitDriveMotor(right);	
	gyro_direct_fd=InitGyro(gyro);
		
	laser_fd=InitLaser(LASER_TTY, old_io);

	InitLaserMotor(laser_motor, duty_cycle);
	SynchronizeLaser(laser_fd);

	ScanLoop(laser_fd, gyro_direct_fd, left, right, dest_fd, si_dest);

	close(dest_fd);
	close(gyro_direct_fd);
	
	laser_motor.stop();	
	CloseLaser(laser_fd, old_io);
	fflush(stdout);
	
	return 0;
}

void ProcessArguments(int argc, char **argv, int &duty_cycle)
{
	if(argc<2)
	{
		printf("Assuming default duty cycle (-42)\n");
		fflush(stdout);
		duty_cycle=-42;
		return;
	}
	
	duty_cycle=atoi(argv[1]);
}

void SetDeviceAttribute(const char *device, const char *attribute, bool fail_silently)
{
	int fd;

	if((fd=open(device, O_WRONLY))==-1)
	{
		if(!fail_silently)
			Die("open(device, O_WRONLY))");
		return;
	}
	if( (write(fd, attribute, strlen(attribute)))!=(int)strlen(attribute))
	{
		if(!fail_silently)
			Die(attribute);
		return;
	}
	
	printf("%s set to %s\n", device, attribute);
	fflush(stdout);
	
	close(fd);
}


int InitLaser(const char* device, termios &old_io)
{
	int fd;
	termios io;

	SetDeviceAttribute(LASER_PORT_MODE, LASER_UART_MODE);
	
	if ((fd=open(device, O_RDONLY))==-1)
		Die("open(device, O_RDONLY)");
	
	if(tcgetattr(fd, &old_io) < 0)
		Die("tcgetattr(fd, &old_io)");
			
	io.c_iflag=io.c_oflag=io.c_lflag=0;
	io.c_cflag=CS8|CREAD|CLOCAL; //8 bit characters
	
	io.c_cc[VMIN]=1; //one input byte enough
	io.c_cc[VTIME]=0; //no timer
	
	if(cfsetispeed(&io, B115200) < 0 || cfsetospeed(&io, B115200) < 0)
		Die("cfsetispeed(&io, B115200) < 0 || cfsetospeed(&io, B115200) < 0");
 
	if(tcsetattr(fd, TCSAFLUSH, &io) < 0)
		Die("tcsetattr(fd, TCSAFLUSH, &io)");
		
	printf("UART initialized\n");		
	fflush(stdout);
	return fd;
}

void CloseLaser(int fd, termios &old_io)
{
	//restore flags
	
	if(tcsetattr(fd, TCSAFLUSH, &old_io) < 0)
		Die("tcsetattr(fd, TCSAFLUSH, &old_io)");
	
	printf("Restoring UART settings\n");
	
	close(fd);
}

void SynchronizeLaser(int fd)
{
	uint8_t c=0;
	int j=0;
	
	while(true)
	{
		if (read(fd,&c,1)>0)
		{
			if( (++j)>=(22*90*10) && c==0xFA)
			{
				if (read(fd,&c,1)>0 && c!=0xA0)
					continue;
					
				for(int i=0;i<20 + 22*89;++i) 
					if (read(fd,&c,1)<0)
						Die("Laser synchroznization failed (21 bytes discard)\n");
		
				termios io;
				if(tcgetattr(fd, &io) < 0)
					Die("tcgetattr(fd, &io)");
			
				io.c_cc[VMIN]=sizeof(laser_frame)*LASER_FRAMES_PER_PACKET; 
				if(tcsetattr(fd, TCSANOW, &io) < 0)
					Die("tcsetattr(fd, TCSANOW, &io)");
			
				//discard some packets to let the laser stabilize
				uint8_t data[sizeof(laser_frame)*LASER_FRAMES_PER_PACKET];
				
				for(int rotation=0;rotation<10;++rotation)
					for(int packet=0;packet<90/LASER_FRAMES_PER_PACKET;++packet)
					{
						size_t bytes_read=0;
						int status=0;
						
						while( bytes_read < sizeof(laser_frame)*LASER_FRAMES_PER_PACKET )
						{
							if( (status=read(fd,data+bytes_read,sizeof(laser_frame)*LASER_FRAMES_PER_PACKET-bytes_read))<0 )
								Die("status=read(fd,data+bytes_read,sizeof(laser_frame)*LASER_FRAMES_PER_PACKET-bytes_read))<0");
							else if(status==0)
								Die("status==0, read no bytes?\n");
								
							bytes_read+=status;
						}						
					}
						
				printf("\nLaser synchronized\n");
				fflush(stdout);
				break;
			}	
		}
		else
			Die("Laser synchronization failed\n");
	}
}

uint16_t Checksum(const uint8_t data[20])
{
	uint32_t chk32=0;
	uint16_t word;
	
	for(int i=0;i<10;++i)
	{
		word=data[2*i] + (data[2*i+1] << 8);
		chk32 = (chk32 << 1) + word;
	}
	
	uint32_t checksum=(chk32 & 0x7FFF) + (chk32 >> 15);
	return checksum & 0x7FFF;
}
 
bool ReadLaser(int fd, laser_frame frame[LASER_FRAMES_PER_PACKET])
{
	uint8_t data[sizeof(laser_frame)*LASER_FRAMES_PER_PACKET]; //22 bytes
	size_t bytes_read=0;
	int status=0;
	
	while( bytes_read < sizeof(laser_frame)*LASER_FRAMES_PER_PACKET )
	{
		if( (status=read(fd,data+bytes_read,sizeof(laser_frame)*LASER_FRAMES_PER_PACKET-bytes_read))<0 )
		{
			if(errno==EINTR) //interrupted by signal
				return 0;
			Die("status=read(fd,data+bytes_read,sizeof(laser_frame)*LASER_FRAMES_PER_PACKET-bytes_read))<0");
		}
		else if(status==0)
			Die("status==0, read no bytes?\n");
			
		bytes_read+=status;
	}

	
	memcpy(frame, data, LASER_FRAMES_PER_PACKET*sizeof(laser_frame));
	
	for(int i=0;i<LASER_FRAMES_PER_PACKET;++i)
	{
		if(Checksum(data+i*sizeof(laser_frame))!=frame[i].checksum)
		{
			fprintf(stderr, " CRCFAIL ");			
		}
		if(frame[i].start!=0xFA)
			Die("Synchronization lost!\n");
	}
	return 1;
}



void ScanLoop(int laser_fd, int gyro_direct_fd, const large_motor& left, const large_motor &right, int dest_fd, const sockaddr_in &dst)
{
	robot_frame rframe;
	laser_frame frame[LASER_FRAMES_PER_PACKET];

	rframe.left_end=rframe.right_end=rframe.angle_stop=0;
	uint32_t rpm=0;
	std::chrono::time_point<std::chrono::high_resolution_clock> start, end=std::chrono::high_resolution_clock::now();
	std::chrono::duration<float, std::micro> elapsed;
		
	for(uint32_t counter=1;g_finish_program==0;++counter)
	{
		start=end;
		rframe.left_start=rframe.left_end;
		rframe.right_start=rframe.right_end;
		rframe.angle_start=rframe.angle_stop;
		
		if(ReadLaser(laser_fd, frame)==0)
			break;
						
		end=std::chrono::high_resolution_clock::now();
		elapsed=end-start;
		rframe.elapsed_ms=elapsed.count()/10; 
		
		rframe.left_end=left.position();
		rframe.right_end=right.position();
		
		rframe.laser_angle=(frame[0].index-0xA0);

		rpm=0;
		for(int i=0;i<LASER_FRAMES_PER_PACKET;++i)
		{
			rpm+=frame[i].speed;
			memcpy(rframe.laser_readings+4*i, frame[i].readings, 4*sizeof(laser_reading));
		}
		
		rframe.laser_speed=rpm/LASER_FRAMES_PER_PACKET/64;
		
		rframe.angle_stop=ReadGyroAngle(gyro_direct_fd);
		SendReadingsFrame(dest_fd, dst, rframe);	
	}
}

void SendReadings(int fd, const sockaddr_in &dst, const laser_reading readings[360])
{
	int result;
	if ((result = sendto(fd, readings, sizeof(laser_reading)*360, 0, (sockaddr*)&dst, sizeof(dst))) == -1)
		Die("sendto failed");
	
	if(result!=360*sizeof(laser_reading))
		Die("sendto incomplete packet sent!");
}

void SendReadingsFrame(int fd, const sockaddr_in &dst, const robot_frame &frame)
{
	int result;
	if ((result = sendto(fd, &frame, sizeof(robot_frame), 0, (sockaddr*)&dst, sizeof(dst))) == -1)
		Die("sendto failed");
	
	if(result!=sizeof(robot_frame))
		Die("sendto incomplete packet sent!");	
}

void Finish(int signal)
{
	g_finish_program=1;
}

void RegisterSignals()
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler=Finish;
	if(sigaction(SIGTERM, &action, NULL)==-1)
		Die("sigaction(SIGTERM, &action, NULL)");
	if(sigaction(SIGINT, &action, NULL)==-1)
		Die("sigaction(SIGINT, &action, NULL)");
	if(sigaction(SIGQUIT, &action, NULL))
		Die("sigaction(SIGQUIT, &action, NULL)");	
	if(sigaction(SIGHUP, &action, NULL))
		Die("sigaction(SIGHUP, &action, NULL)");	
}

void InitLaserMotor(motor& m, int duty_cycle)
{
	if(!m.connected())
		Die("Motor not connected");

	m.set_speed_regulation_enabled(motor::speed_regulation_off);
	m.set_stop_command(motor::stop_command_coast);	
	m.set_duty_cycle_sp(duty_cycle);
	m.run_direct();
}

void InitDriveMotor(large_motor& m)
{
	if(!m.connected())
		Die("Motor not connected");

	m.set_position(0);
}

int InitGyro(sensor& gyro)
{
	int fd;
	
	if(!gyro.connected())	
		Die("Gyroscope not connected");
		
	//gyro.set_mode("ALL");
	gyro.set_command("RESET");
	
	printf("Callculating gyroscope bias drift\n");
	fflush(stdout);
	Sleep(500);
	SetDeviceAttribute(GYRO_POLL_MS, GYRO_POLL_MS_SETTING);

	if((fd=open(GYRO_DIRECT, O_RDONLY))==-1)	
		Die("open(GYRO_DIRECT, O_RDONLY)");
		
	return fd;
}

int16_t ReadGyroAngle(int gyro_direct_fd)
{
	int16_t val;
	if(lseek(gyro_direct_fd, 0x42, SEEK_SET)==-1)
		Die("lseek(gyro_direct_fd, 0x42, SEEK_SET)==-1");
		
	if(read(gyro_direct_fd, (char*)(&val), 2)!=2)
		Die("gyro_direct fd\n");
	
	return val;
}


int InitSocketUDP(int port)
{
	struct sockaddr_in si_me;
	int s;

    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)    
        Die("socket");
    		
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
	//prepare address 
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
        Die("bind");
    
	return s;
}

void InitDestination(sockaddr_in &si_dest, const char *host, int port)
{
	si_dest.sin_family = AF_INET;
	if (!inet_pton(AF_INET, host, &si_dest.sin_addr))
		Die("inet_pton");
	si_dest.sin_port = htons(port);
}


void Sleep(int ms)
{	
	this_thread::sleep_for(chrono::milliseconds(ms));
}
void Die(const char *s)
{
    perror(s);
    exit(1);
}
