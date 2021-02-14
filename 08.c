/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <errno.h>
#include <limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Upper bound to how many digits a given type may hold */
#define DIGITS(T) (CHAR_BIT * 10 * sizeof(T) / (9 * sizeof(char)))

typedef enum { ACC, JMP, NOP } Operation;

typedef struct {
	Operation op;
	intmax_t x;
} Instruction;

typedef enum { NO_RUN, LOOPED, TERMINATED } RunResult;

static regex_t reg;
static Instruction *instr = NULL;
static size_t cinstr = 0, ninstr = 0;

static void
freedata(void)
{
	regfree(&reg);
	free(instr);
}

static Operation
parseop(const char *const str)
{
	if (strcmp(str, "acc") == 0)
		return ACC;
	else if (strcmp(str, "jmp") == 0)
		return JMP;
	return NOP;
}

static bool
resizeinstructions(void)
{
	if (ninstr < cinstr)
		return true;
	if (cinstr >= SIZE_MAX / 2)
		return false;
	cinstr = (cinstr > 0)? 2 * cinstr : 1;
	Instruction * const new = realloc(instr, cinstr * sizeof(Instruction));
	if (new == NULL)
		return false;
	instr = new;
	return true;
}

static void
parseerr(const char * const str, const uintmax_t line)
{
	if (errno != 0) {
		char buf[strlen(str) + 10 + DIGITS(uintmax_t)];
		sprintf(buf, "%s on line %ju", str, line);
		perror(buf);
	} else {
		fprintf(stderr, "%s on line %ju\n", str, line);
	}
}

static bool
overflows(const intmax_t x, const intmax_t y)
{
	return (y > 0 && x > INTMAX_MAX - y) || (y < 0 && x < INTMAX_MIN - y);
}

static RunResult
subsrun(const size_t s, intmax_t * const acc)
{
	if (s < SIZE_MAX) {
		if (instr[s].op == JMP)
			instr[s].op = NOP;
		else if (instr[s].op == NOP && instr[s].x != 0)
			instr[s].op = JMP;
		else
			return NO_RUN;
	}
	uint8_t beenthere[ninstr / 8 + 1];
	for (size_t i = 0; i < ninstr / 8 + 1; i++)
		beenthere[i] = 0;
	size_t pc = 0;
	while (!(beenthere[pc / 8] & (1u << (pc % 8)))) {
		if (pc >= ninstr) {
			fprintf(stderr, "Program counter (%zu) too big\n", pc);
			exit(EXIT_FAILURE);
		} else if (pc == ninstr - 1) {
			break;
		}
		beenthere[pc / 8] |= 1u << (pc % 8);
		if (instr[pc].op == ACC) {
			if (overflows(*acc, instr[pc].x)) {
				fprintf(stderr,
				        "%jd + %jd overflows\n",
				        *acc,
				        instr[pc].x);
				exit(EXIT_FAILURE);
			}
			*acc += instr[pc].x;
		}
		pc += instr[pc].op == JMP? instr[pc].x : 1;
	}
	if (s < SIZE_MAX)
		instr[s].op = instr[s].op == JMP? NOP : JMP;
	return pc == ninstr - 1? TERMINATED : LOOPED;
}

int
day08(FILE * const in)
{
	if (atexit(freedata) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	int res = regcomp(&reg, "^(acc|jmp|nop) ([+-][0-9]+)$", REG_EXTENDED);
	if (res != 0) {
		const size_t n = regerror(res, &reg, NULL, 0);
		char buf[n];
		regerror(res, &reg, buf, n);
		fprintf(stderr, "Could not compile regex: %s", buf);
		return EXIT_FAILURE;
	}
	uintmax_t line = 1;
	char input[6 + DIGITS(intmax_t)], fmt[17 + DIGITS(uintmax_t)];
	sprintf(fmt, "%%%ju[acjmnop 0-9+-]", DIGITS(size_t));
	while (fscanf(in, fmt, input) == 1) {
		regmatch_t match[3];
		if (regexec(&reg, input, 3, match, 0) == REG_NOMATCH) {
			fprintf(stderr, "Bad input format on line %ju\n", line);
			return EXIT_FAILURE;
		}
		input[match[1].rm_eo] = 0;
		if (!resizeinstructions()) {
			parseerr("Could not reallocate instructions", line);
			return EXIT_FAILURE;
		}
		instr[ninstr].op = parseop(input);
		sscanf(input + match[2].rm_so, "%jd", &instr[ninstr++].x);
		const int next = fgetc(in);
		if (next != '\n' && next != EOF) {
			fprintf(stderr, "Line %ju is too long\n", line);
			return EXIT_FAILURE;
		}
		line++;
	}
	if (!feof(in) || ferror(in)) {
		fputs("Puzzle input parsing failed\n", stderr);
		return EXIT_FAILURE;
	}
	intmax_t acc = 0;
	if (subsrun(SIZE_MAX, &acc) != LOOPED) {
		fputs("Program was supposed to loop but didn't\n", stderr);
		return EXIT_FAILURE;
	}
	printf("Loop\t%jd\n", acc);
	for (size_t i = 0; i < ninstr; i++) {
		acc = 0;
		const RunResult result = subsrun(i, &acc);
		if (result == NO_RUN) {
			continue;
		} else if (result == TERMINATED) {
			printf("No loop\t%jd\n", acc);
			return EXIT_SUCCESS;
		}
	}
	fputs("All substitutions loop\n", stderr);
	return EXIT_FAILURE;
}
