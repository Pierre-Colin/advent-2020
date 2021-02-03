# advent-2020 - C solutions to Advent of Code 2020
# See LICENSE file for copyright and license details.
.POSIX:

CC = cc
BIN = advent
SRC = advent.c 01.c 02.c 03.c 04.c 05.c 06.c 07.c 08.c 09.c 10.c 11.c 12.c 13.c 14.c 15.c 16.c 17.c 18.c 19.c 20.c 21.c 22.c 23.c 24.c 25.c
OBJ = ${SRC:.c=.o}
CFLAGS = -std=c99 -Wall -Wextra -O3
LDFLAGS = -flto

${BIN}: ${OBJ}
	${CC} ${LDFLAGS} -o ${BIN} ${OBJ}

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	rm -f ${OBJ} ${BIN}

.PHONY: clean
