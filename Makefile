# advent-2020 - C solutions to Advent of Code 2020
# See LICENSE file for copyright and license details.
.POSIX:

CC = cc
BIN = advent
SRC = advent.c reportrepair.c passwords.c toboggan.c passports.c boarding.c questions.c bags.c handheld.c
OBJ = ${SRC:.c=.o}
CFLAGS = -std=c99 -Wall -Wextra -O3

${BIN}: ${OBJ}
	${CC} -o ${BIN} ${OBJ}

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	rm -f ${OBJ} ${BIN}

.PHONY: clean
