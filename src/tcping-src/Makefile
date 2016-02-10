#### Uncomment one of the following:
## Visual C++
CC = cl
CFLAGS = -EHsc -Os
#LFLAGS = wsock32.lib /Fe$@
LFLAGS = ws2_32.lib /MACHINE:X86 /Fe$@
OBJEXT = obj

## Borland C++
#CC = bcc32
#CFLAGS = -Os -5
#LFLAGS = -e$@
#OBJEXT = obj

## Cygwin/GNU C++
#CC = g++
#CFLAGS = -Wall -O
#LFLAGS = -lwsock32 -o $@
#OBJEXT = o


.SUFFIXES: .cpp .$(OBJEXT)

## Build rule for VC++ and BC++
.cpp.$(OBJEXT):
	$(CC) -c $(CFLAGS) $<

## Build rule for Cygwin/GNU C++
#.cpp.$(OBJEXT):
#	$(CC) -c $(CFLAGS) $< -o $*.o


#### Major targets #####################################################

all: tcping.exe
ctags:
	ctags *.cpp *.h

clean:
	del *.$(OBJEXT) *.bak

distclean: clean
	del *.exe *.swp


#### Common dependencies ###############################################

main.$(OBJEXT): main.cpp
ws-util.$(OBJEXT): ws-util.cpp ws-util.h


#### tcping ##############################################

TCPING_OBJS = tcping.$(OBJEXT) main.$(OBJEXT) \
		ws-util.$(OBJEXT)
tcping.exe: $(TCPING_OBJS)
	$(CC) $(TCPING_OBJS) $(LFLAGS)

tcping.$(OBJEXT): tcping.cpp ws-util.h


