CC = gcc
CFLAGS = -Wall -pedantic
DEBUG_FLAGS = -Wall -pedantic -g

PREFIX = /usr/local

SRC = jgrep.c regex.c
DEBUG_SRC = jgrep.debug.c regex.debug.c

all:
	@echo jgrep build flags: ${CFLAGS}
	${CC} ${CFLAGS} -o jgrep ${SRC}

clean:
	rm -f jgrep debug

debug:
	@echo debug build flags: ${DEBUG_FLAGS}
	cat jgrep.c > jgrep.debug.c
	sed '/char\* post = parse(exp)\;/a\
		\tfprintf(stderr, "postfixed expr: %s\\n", post);' \
		regex.c > regex.debug.c
	${CC} ${DEBUG_FLAGS} -o debug ${DEBUG_SRC}
	rm -f ${DEBUG_SRC}

install: all
	cp -f jgrep ${PREFIX}/bin
	chmod 755 ${PREFIX}/bin/jgrep

uninstall:
	rm -f ${PREFIX}/bin/jgrep

.PHONY: all clean debug install uninstall
