CC = gcc
CFLAGS = -Wall -pedantic

PREFIX = /usr/local

SRC = jgrep.c regex.c

all:
	@echo jgrep build flags: ${CFLAGS}
	${CC} ${CFLAGS} -o jgrep ${SRC}

clean:
	rm -f jgrep

install: all
	cp -f jgrep ${PREFIX}/bin
	chmod 755 ${PREFIX}/bin/jgrep

uninstall:
	rm -f ${PREFIX}/bin/jgrep
