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

static int
compumax(const void *x, const void *y)
{
	const uintmax_t xx = *(const uintmax_t *) x;
	const uintmax_t yy = *(const uintmax_t *) y;
	if (xx < yy)
		return -1;
	else if (xx > yy)
		return 1;
	return 0;
}

static void
resizearr(uintmax_t ** restrict jolts, size_t * restrict c, const size_t n)
{
	if (n < *c)
		return;
	if (*c >= SIZE_MAX / 2) {
		fprintf(stderr, "Doubling %zu causes wraparound\n", *c);
		free(*jolts);
		exit(EXIT_FAILURE);
	}
	*c = *c > 0? 2 * *c : 1;
	uintmax_t * const new = realloc(*jolts, *c * sizeof(uintmax_t));
	if (new == NULL) {
		fprintf(stderr, "Could not resize array to %zu\n", *c);
		free(*jolts);
		exit(EXIT_FAILURE);
	}
	*jolts = new;
}

int
day10(FILE * const in)
{
	uintmax_t *jolts = NULL, input;
	size_t num = 0, cap = 0;
	while (fscanf(in, "%ju", &input) == 1) {
		resizearr(&jolts, &cap, num);
		jolts[num++] = input;
		const int next = fgetc(in);
		if (next != '\n' && next != EOF)
			break;
	}
	if (!feof(in) || ferror(in)) {
		fputs("Puzzle input parsing failed\n", stderr);
		free(jolts);
		return EXIT_FAILURE;
	}
	qsort(jolts, num, sizeof(uintmax_t), compumax);
	resizearr(&jolts, &cap, num);
	jolts[num] = jolts[num - 1] + 3;
	uintmax_t jump1 = 0, jump3 = 0;
	for (size_t i = 0; i + 1 < num; i++) {
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
	printf("Part 1\t%ju\n", jump1 * jump3);
	uintmax_t ways[num + 1];
	memset(ways, 0, (num + 1) * sizeof(uintmax_t));
	for (size_t i = 0; i < num + 1; i++) {
		if (jolts[i] <= 3)
			ways[i]++;
		for (size_t j = i - 1; j < i && jolts[j] + 3 >= jolts[i]; j--)
			ways[i] += ways[j];
	}
	free(jolts);
	printf("Part 2\t%ju\n", ways[num]);
	return EXIT_SUCCESS;
}
