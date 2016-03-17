TARGET = xv11pipe-loop.exe
CSHARP = csc #for Microsoft Visual C# Compiler (use from Developer Command Prompt for Visual Studio)
#CSHARP = mcs #for Mono, uncomment if needed (use from Mono Command Prompt)

default: $(TARGET)
all: default

$(TARGET): Program.cs LaserReading.cs
	$(CSHARP) Program.cs LaserReading.cs -out:$(TARGET)
