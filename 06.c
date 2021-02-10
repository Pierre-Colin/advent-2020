/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define DIGITS(T) (CHAR_BIT * 10 * sizeof(T) / (9 * sizeof(char)))

static void
updatetotals(const uint_fast32_t acurrent,
             uintmax_t * const restrict atotal,
             const uint_fast32_t ecurrent,
             uintmax_t * const restrict etotal)
{
	for (uint_fast8_t i = 0; i < 26; i++) {
		if (acurrent & (1u << i))
			(*atotal)++;
		if (ecurrent & (1u << i))
			(*etotal)++;
	}
}

static void
parseerr(const uintmax_t line)
{
	if (errno != 0) {
		char buf[19 + DIGITS(uintmax_t)];
		sprintf(buf, "Bad input on line %ju", line);
		perror(buf);
	} else {
		fprintf(stderr, "Bad input on line %ju\n", line);
	}
}

int
day06(FILE * const in)
{
	uintmax_t atotal = 0, etotal = 0, line = 1;
	uint_fast32_t acurrent = 0, ecurrent = 0x03ffffff;
	errno = 0;
	while (!feof(in) && !ferror(in)) {
		char input[27];
		if (fscanf(in, "%26[a-z]", input) == 1) {
			uint_fast32_t this = 0;
			for (const char *c = input; *c != 0; c++)
				this |= UINT32_C(1) << (*c - 'a');
			acurrent |= this;
			ecurrent = 0x03ffffff & (ecurrent & this);
			const int end = fgetc(in);
			if (end != EOF && end != '\n') {
				parseerr(line);
				return EXIT_FAILURE;
			} else if (end == EOF) {
				updatetotals(acurrent,
				             &atotal,
				             ecurrent,
				             &etotal);
			}
		} else {
			const int c = fgetc(in);
			if (c != EOF && c != '\n') {
				parseerr(line);
				return EXIT_FAILURE;
			}
			updatetotals(acurrent, &atotal, ecurrent, &etotal);
			acurrent = 0;
			ecurrent = 0x03ffffff;
		}
		line++;
	}
	if (!feof(in) || ferror(in)) {
		if (errno != 0)
			perror("Puzzle input parsing failed");
		else
			fputs("Puzzle input parsing failed\n", stderr);
		return EXIT_FAILURE;
	}
	printf("Any\t%ju\nEvery\t%ju\n", atotal, etotal);
	return EXIT_SUCCESS;
}
