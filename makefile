# MMAFlash GCC makefile
CC=gcc
LD=ld
EXEC_FILE=MMAFlash.exe
LIB_PATH?=.
CFLAGS= -mno-stack-arg-probe -Os -Wall -pedantic -mwindows -I.
LDFLAGS= --strip-all --subsystem windows -L $(LIB_PATH) -l kernel32 -l user32 -l ole32 -l comctl32 -l comdlg32 -l shell32 -l gdi32 -nostdlib --exclude-libs msvcrt.a -e_WinMain@16

OBJ_EXT=.o
OBJS=SysToolX${OBJ_EXT} FlashKit${OBJ_EXT} MMAFlash${OBJ_EXT}

all: $(EXEC_FILE)

$(EXEC_FILE): $(OBJS)
	${LD} ${OBJS} resource/MMAFlash.res -o $@ ${LDFLAGS}


o=${OBJ_EXT}

wipe:
	@rm -rf *${OBJ_EXT}

clean:	wipe all
