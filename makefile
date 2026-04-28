# OpenQT Makefile for Watcom C/C++
# Hebrew Word Processor - QText 5.5 Clone

# Compiler settings
CC = wcl
CC386 = wcl386

# 16-bit DOS target (real mode)
CFLAGS16 = -bt=dos -ml -ox -w3 -zq
LFLAGS16 = -bt=dos -ml -fe=openqt.exe

# 32-bit DOS target (DOS/4GW protected mode)
CFLAGS32 = -bt=dos -ox -w3 -zq
LFLAGS32 = -bt=dos -l=dos4g -fe=openqt32.exe

# Source files
SRC = openqt.c

# Default target - 16-bit version
all: openqt.exe

# 16-bit DOS executable (runs on any DOS system)
openqt.exe: $(SRC)
	$(CC) $(CFLAGS16) $(LFLAGS16) $(SRC)

# 32-bit DOS/4GW executable (requires DOS4GW.EXE)
openqt32.exe: $(SRC)
	$(CC386) $(CFLAGS32) $(LFLAGS32) $(SRC)

# Build both versions
both: openqt.exe openqt32.exe

# Clean build files
clean: .SYMBOLIC
	@if exist *.obj del *.obj
	@if exist *.exe del *.exe
	@if exist *.err del *.err
	@if exist *.map del *.map

# Install to destination
install: openqt.exe
	@if not exist $(DEST) mkdir $(DEST)
	copy openqt.exe $(DEST)
	copy HEBVGA.COM $(DEST)

.SYMBOLIC
