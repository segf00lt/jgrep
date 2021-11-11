CC = gcc
CFLAGS = -Wall -pedantic
DEBUGFLAGS = -Wall -pedantic -g

PREFIX = /usr/local

SRC = jgrep.c regex.c

all:
	@echo jgrep build flags: ${CFLAGS}
	${CC} ${CFLAGS} -o jgrep ${SRC}

clean:
	rm -f jgrep debug

debug:
	@echo debug build flags: ${DEBUGFLAGS}
	${CC} ${DEBUGFLAGS} -o debug ${SRC}

install: all
	cp -f jgrep ${PREFIX}/bin
	chmod 755 ${PREFIX}/bin/jgrep

uninstall:
	rm -f ${PREFIX}/bin/jgrep
