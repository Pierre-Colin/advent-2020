/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 128

static int
compu8(const void *x, const void *y)
{
	const uint8_t xx = *(const uint8_t *) x;
	const uint8_t yy = *(const uint8_t *) y;
	if (xx < yy)
		return -1;
	else if (xx > yy)
		return 1;
	return 0;
}

int
day10(FILE * const in)
{
	uint8_t jolts[MAX_LINES];
	size_t n = 0;
	int scanres;
	while ((scanres = fscanf(in, "%" SCNu8 "%*1[\n]", jolts + n)) != EOF) {
		if (scanres < 1) {
			if (errno != 0)
				perror("Failed to input");
			else
				fputs("Bad input format\n", stderr);
			return EXIT_FAILURE;
		}
		if (++n == MAX_LINES) {
			fputs("Maximum input lines reached\n", stderr);
			return EXIT_FAILURE;
		}
	}
	qsort(jolts, n, sizeof(uint8_t), compu8);
	jolts[n] = jolts[n - 1] + 3;
	uint16_t jump1 = 0, jump3 = 0;
	for (size_t i = 0; i + 1 < n; i++) {
		if (jolts[i] + 1 == jolts[i + 1])
			jump1++;
		else if (jolts[i] + 3 == jolts[i + 1])
			jump3++;
	}
	if (jolts[0] == 1)
		jump1++;
	if (jolts[0] == 3)
		jump3++;
	jump3++;
	printf("Part 1\t%" PRIu16 "\n", jump1 * jump3);
	uint64_t ways[n + 1];
	memset(ways, 0, (n + 1) * sizeof(uint64_t));
	for (size_t i = 0; i < n + 1; i++) {
		if (jolts[i] <= 3)
			ways[i]++;
		for (size_t j = i - 1; j < i && jolts[j] + 3 >= jolts[i]; j--)
			ways[i] += ways[j];
	}
	printf("Part 2\t%" PRIu64 "\n", ways[n]);
	return EXIT_SUCCESS;
}
