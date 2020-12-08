/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <errno.h>
#include <inttypes.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES ((size_t) 1024)

typedef enum {
	ACC,
	JMP,
	NOP
} Operation;

typedef struct {
	Operation op;
	int64_t x;
} Instruction;

typedef enum {
	NO_RUN,
	LOOPED,
	TERMINATED
} RunResult;

static regex_t codepattern;

static void
freedata(void)
{
	regfree(&codepattern);
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

static int64_t
parsei64(const char *const str, const regmatch_t match)
{
	int64_t x = 0;
	for (regoff_t i = match.rm_so + 1; i < match.rm_eo; i++) {
		if (x > (INT64_MAX - str[i] + '0') / 10) {
			fprintf(stderr,
			        "Operand %s out of bounds\n",
			        str + match.rm_so);
			return EXIT_FAILURE;
		}
		x = 10 * x + str[i] - '0';
	}
	if (str[match.rm_so] == '-')
		return -x;
	return x;
}

static void
execinstr(const Instruction *const instructions,
          uint8_t *const beenthere,
          size_t *const pc,
          int64_t *const acc)
{
	beenthere[*pc / 8] |= 1u << (*pc % 8);
	switch (instructions[*pc].op) {
	case ACC:
		*acc += instructions[*pc].x;
		/* FALLTHROUGH */
	case NOP:
		(*pc)++;
		break;
	case JMP:
		*pc += instructions[*pc].x;
	}
}

static RunResult
subsrun(Instruction *const instructions,
        const size_t ninstr,
        const size_t s,
        int64_t *const acc)
{
	if (s < MAX_LINES) {
		if (instructions[s].op == JMP)
			instructions[s].op = NOP;
		else if (instructions[s].op == NOP && instructions[s].x != 0)
			instructions[s].op = JMP;
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
		execinstr(instructions, beenthere, &pc, acc);
	}
	if (s < MAX_LINES) {
		if (instructions[s].op == JMP)
			instructions[s].op = NOP;
		else
			instructions[s].op = JMP;
	}
	if (pc == ninstr - 1)
		return TERMINATED;
	else
		return LOOPED;
}

int
day08(void)
{
	if (atexit(freedata) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	int res = regcomp(&codepattern,
	                  "^(acc|jmp|nop) ([+-][0-9]+)$",
	                  REG_EXTENDED);
	if (res != 0) {
		char buf[128];
		regerror(res, &codepattern, buf, 128);
		fprintf(stderr, "Could not compile regex: %s", buf);
		return EXIT_FAILURE;
	}
	Instruction instructions[MAX_LINES];
	size_t ninstr = 0;
	char input[17];
	while ((res = scanf("%16[acjmnop 0-9+-]%*1[\n]", input)) != EOF) {
		if (res < 1) {
			if (errno != 0)
				perror("Falied to read input");
			else
				fputs("Bad input format\n", stderr);
			return EXIT_FAILURE;
		}
		regmatch_t match[3];
		if (regexec(&codepattern, input, 3, match, 0) == REG_NOMATCH) {
			fputs("Bad input format\n", stderr);
			return EXIT_FAILURE;
		}
		input[match[1].rm_eo] = 0;
		if (ninstr == MAX_LINES) {
			fprintf(stderr,
			        "%zu instruction limit reached\n",
			        MAX_LINES);
			return EXIT_FAILURE;
		}
		instructions[ninstr++] = (Instruction) {
			.op = parseop(input),
			.x = parsei64(input, match[2])
		};
	}
	int64_t acc = 0;
	switch (subsrun(instructions, ninstr, MAX_LINES, &acc)) {
	case LOOPED:
		printf("Loop\t%" PRIi64 "\n", acc);
		break;
	default:
		fputs("Program was supposed to loop but didn't\n", stderr);
		return EXIT_FAILURE;
	}
	for (size_t i = 0; i < ninstr; i++) {
		acc = 0;
		const RunResult result = subsrun(instructions, ninstr, i, &acc);
		if (result == NO_RUN) {
			continue;
		} else if (result == TERMINATED) {
			printf("No loop\t%" PRIi64 "\n", acc);
			return EXIT_SUCCESS;
		}
	}
	fputs("All substitutions loop\n", stderr);
	return EXIT_FAILURE;
}

