# GCC mingw32-make makefile
CC=@gcc
RC=@windres
RM=@rm -rf

# to fix "undefined reference to `_alloca'" add "-mno-stack-arg-probe"
# http://cygwin.cygwin.narkive.com/XE6kJpcC/strange-behaviour-of-gcc
CFLAGS= -mno-stack-arg-probe -fno-exceptions -fno-rtti -Os -Wall -pedantic -I.
LDFLAGS= -s -nostdlib -mwindows -e_WinMain@16
LDLIBS= -l kernel32 -l user32 -l ole32 -l comctl32 -l comdlg32 -l shell32 -l gdi32

EXE=MMAFlash
RES=${EXE}.res
EXT=.o
OBJ=SysToolX${EXT} FlashKit${EXT} MMAFlash${EXT}

$(EXE):	$(OBJ) $(RES)

$(RES):	resource/${EXE}.rc resource/${EXE}.h
	${RC} --include-dir=resource -i resource/${EXE}.rc -O coff -o $@

wipe:
	${RM} *${EXT}
	${RM} ${RES}

all:	$(EXE)

clean:	wipe all
