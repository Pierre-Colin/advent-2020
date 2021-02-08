/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void
parseseatpart(uint_fast16_t * const restrict id,
              const char * const restrict input,
              const uint_fast8_t begin,
              const uint_fast8_t end,
              const char high,
              const char low)
{
	for (uint_fast8_t i = begin; i < end; i++) {
		*id *= 2;
		if (input[i] == high) {
			(*id)++;
		} else if (input[i] != low) {
			fputs("Bad input format\n", stderr);
			exit(EXIT_FAILURE);
		}
	}
}

int
day05(FILE * const in)
{
	uint_fast16_t highest = 0;
	uint_fast8_t present[128] = { 0 };
	char input[11];
	errno = 0;
	while (fscanf(in, "%10[FBLR]", input) == 1) {
		uint_fast16_t id = 0;
		parseseatpart(&id, input, 0, 7, 'B', 'F');
		parseseatpart(&id, input, 7, 10, 'R', 'L');
		if (id > highest)
			highest = id;
		present[id / 8] |= 1 << (id % 8);
		if (fgetc(in) != '\n')
			break;
	}
	if (!feof(in) || ferror(in)) {
		if (errno != 0)
			perror("Failed to input boarding pass");
		else
			fputs("Bad input format\n", stderr);
		return EXIT_FAILURE;
	}
	printf("Highest\t%" PRIuFAST16 "\n", highest);
	bool began = false;
	for (uint_fast16_t i = 0; i < 128; i++) {
		for (uint_fast8_t j = 0; j < 8; j++) {
			if (!began && (present[i] & (1 << j)))
				began = true;
			if (began && !(present[i] & (1 << j))) {
				printf("Seat\t%" PRIuFAST16 "\n", 8 * i + j);
				return EXIT_SUCCESS;
			}
		}
	}
	fputs("Seat not found\n", stderr);
	return EXIT_FAILURE;
}
