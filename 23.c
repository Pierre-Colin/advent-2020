/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static bool
pickedup(const uint_fast32_t dest, const uint_fast32_t pickup[const 3])
{
	return (pickup[0] == dest) | (pickup[1] == dest) | (pickup[2] == dest);
}

static void
play(const uint_fast32_t ncups,
     uint_fast32_t cups[ncups],
     uint_fast32_t current,
     uint_fast32_t moves)
{
	while (moves--) {
		uint_fast32_t pickup[3] = {
			cups[current],
			cups[cups[current]],
			cups[cups[cups[current]]]
		};
		uint_fast32_t dest = current > 0? current - 1: ncups - 1;
		while (pickedup(dest, pickup))
			dest = dest == 0? ncups - 1 : dest - 1;
		cups[current] = cups[pickup[2]];
		const uint_fast32_t temp = cups[dest];
		cups[dest] = pickup[0];
		cups[pickup[2]] = temp;
		current = cups[current];
	}
}

static void
label(uint_fast8_t ncups,
      const uint_fast8_t icups[const restrict ncups],
      char out[const restrict ncups])
{
	uint_fast32_t cups[ncups];
	for (uint_fast8_t i = 0; i < ncups; i++)
		cups[icups[i] - 1] = icups[(i + 1) % ncups] - 1;
	play(ncups, cups, icups[0] - 1, 100);
	uint_fast8_t c = 0;
	for (uint_fast8_t t = 0; t < ncups - 1; t++) {
		c = cups[c];
		out[t] = c + '1';
	}
	out[ncups - 1] = 0;
}

static uint_fast64_t
stars(const uint_fast32_t ncups,
      const uint_fast8_t icups[const ncups])
{
	/* Too big to fit properly on the stack -- detected by valgrind */
	uint_fast32_t * const cups = malloc(1000000 * sizeof(uint_fast32_t));
	if (cups == NULL) {
		fputs("Could not allocate the cup array\n", stderr);
		exit(EXIT_FAILURE);
	}
	for (uint_fast32_t i = 0; i < 999999; i++) {
		const uint_fast32_t x = i < ncups? icups[i] - 1u : i;
		cups[x] = i < ncups - 1u? icups[i + 1] - 1u : i + 1;
	}
	cups[999999] = icups[0] - 1;
	play(1000000, cups, icups[0] - 1, UINT32_C(10000000));
	uint_fast64_t a = cups[0] + 1, b = cups[cups[0]] + 1;
	free(cups);
	if (a >= UINT_FAST64_MAX / b) {
		fprintf(stderr,
		        "%" PRIuFAST64 " * %" PRIuFAST64 " wraps around\n",
		        a,
		        b);
		exit(EXIT_FAILURE);
	}
	return a * b;
}

int
day23(FILE * const in)
{
	uint_fast8_t cups[9], ncups = 0;
	int c;
	while ((c = fgetc(in)) != EOF && ncups <= 9) {
		if (isspace(c))
			continue;
		if (!isdigit(c)) {
			fprintf(stderr, "%c is not a digit\n", c);
			return EXIT_FAILURE;
		}
		cups[ncups++] = c - '0';
	}
	if (ferror(in) || !feof(in)) {
		fputs("Puzzle input parsing failed\n", stderr);
		return EXIT_FAILURE;
	}
	if (ncups <= 3) {
		fprintf(stderr, "%" PRIuFAST8 " cups is not enough\n", ncups);
		return EXIT_FAILURE;
	}
	uint_fast16_t found = 0;
	for (uint_fast8_t i = 0; i < ncups; i++)
		found |= 1u << (cups[i] - 1);
	if (found != (1u << ncups) - 1) {
		fprintf(stderr,
		        "Cups between 1 and %" PRIuFAST8 " are missing\n",
		        ncups);
		return EXIT_FAILURE;
	}
	char out[ncups];
	label(ncups, cups, out);
	printf("Labels\t%s\n", out);
	printf("Stars\t%" PRIuFAST64 "\n", stars(ncups, cups));
	return EXIT_SUCCESS;
}
