JFLAGS = -cp src -d .
JC = javac
JAR = jar 
JARFLAGS = cfm

.PHONY: default all clean

XV11PIPELOOP_CLASS = com\bmegli\xv11lidar\examples\xv11pipeloop\Main.class
XV11PIPELOOP_JAVA = src\com\bmegli\xv11lidar\examples\xv11pipeloop\Main.java 
XV11PIPELOOP_JAR = xv11pipe-loop.jar
XV11PIPELOOP_MANIFEST = xv11pipe-loop-manifest

LASERREADING_CLASS = com\bmegli\xv11lidar\LaserReading.class
LASERREADING_JAVA = src\com\bmegli\xv11lidar\LaserReading.java
LASERFRAME_CLASS = com\bmegli\xv11lidar\LaserFrame.class
LASERFRAME_JAVA = src\com\bmegli\xv11lidar\LaserFrame.java
LASERSCANNER_CLASS = com\bmegli\xv11lidar\LaserScanner.class
LASERSCANNER_JAVA = src\com\bmegli\xv11lidar\LaserScanner.java

ALL_CLASSES = $(XV11PIPELOOP_CLASS) $(LASERSCANNER_CLASS) $(LASERFRAME_CLASS) $(LASERREADING_CLASS)

default: $(XV11PIPELOOP_JAR)
all: default

$(XV11PIPELOOP_JAR): $(ALL_CLASSES) $(XV11PIPELOOP_MANIFEST)
	$(JAR) $(JARFLAGS) $(XV11PIPELOOP_JAR) $(XV11PIPELOOP_MANIFEST) $(ALL_CLASSES)
		
$(XV11PIPELOOP_CLASS): $(XV11PIPELOOP_JAVA)
	$(JC) $(JFLAGS) $(XV11PIPELOOP_JAVA)

$(LASERSCANNER_CLASS): $(LASERSCANNER_JAVA)
	$(JC) $(JFLAGS) $(LASERSCANNER_JAVA)

$(LASERFRAME_CLASS): $(LASERFRAME_JAVA)
	$(JC) $(JFLAGS) $(LASERFRAME_JAVA)
	
$(LASERREADING_CLASS): $(LASERREADING_JAVA)
	$(JC) $(JFLAGS) $(LASERREADING_JAVA)
	
clean:
	$(RM) $(ALL_CLASSES) $(XV11PIPELOOP_JAR)
	