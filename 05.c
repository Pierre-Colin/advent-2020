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
parseseatpart(uint16_t *const id,
              const char *const input,
              const uint8_t begin,
              const uint8_t end,
              const char high,
              const char low)
{
	for (uint8_t i = begin; i < end; i++) {
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
	uint16_t highest = 0;
	uint8_t present[128] = { 0 };
	char input[11];
	int scanres;
	while ((scanres = fscanf(in, "%10[FBLR]\n", input)) != EOF) {
		if (scanres < 1) {
			if (errno != 0)
				perror("Failed to input boarding pass");
			else
				fputs("Bad input format\n", stderr);
			return EXIT_FAILURE;
		}
		uint16_t id = 0;
		parseseatpart(&id, input, 0, 7, 'B', 'F');
		parseseatpart(&id, input, 7, 10, 'R', 'L');
		if (id > highest)
			highest = id;
		const div_t d = div(id, 8);
		present[d.quot] |= 1 << d.rem;
	}
	printf("Highest\t%" PRIu16 "\n", highest);
	bool began = false;
	for (uint16_t i = 0; i < 128; i++) {
		for (uint8_t j = 0; j < 8; j++) {
			if (!began && (present[i] & (1 << j)))
				began = true;
			if (began && !(present[i] & (1 << j))) {
				printf("Seat\t%" PRIu16 "\n", 8 * i + j);
				return EXIT_SUCCESS;
			}
		}
	}
	fputs("Seat not found\n", stderr);
	return EXIT_FAILURE;
}
