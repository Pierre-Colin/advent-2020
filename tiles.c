/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <inttypes.h>

/* Upper bound to how many digits a given type may hold */
#define DIGITS(T) (CHAR_BIT * 10 * sizeof(T) / (9 * sizeof(char)))

#define PRINT_ERR(s) \
	if (errno != 0) \
		perror(s); \
	else \
		fputs(s "\n", stderr);

static bool *tile = NULL;
static size_t width = 1, height = 1;
static size_t refx = 0, refy = 0;

static size_t
doublewidth(const size_t x)
{
	if (width >= (SIZE_MAX - 1) / 2) {
		fprintf(stderr,
		        "Cannot reallocate tiles: 2 * %zu + 1 wraps around\n",
		        width);
		exit(EXIT_FAILURE);
	}
	const size_t newwidth = 2 * width + 1, quarter = width / 2;
	bool * const new = malloc(newwidth * height * sizeof(bool));
	if (new == NULL) {
		PRINT_ERR("Could not reallocate tiles");
		exit(EXIT_FAILURE);
	}
	for (size_t y = 0; y < height; y++) {
		for (size_t x = 0; x < newwidth; x++)
			new[y * newwidth + x] =
				(quarter + 1 <= x && x < quarter + 1 + width)
				&& tile[y * width + x - quarter - 1];
	}
	free(tile);
	tile = new;
	width = newwidth;
	refx += quarter + 1;
	return x + quarter + 1;
}

static size_t
doubleheight(const size_t y)
{
	if (height >= (SIZE_MAX - 1) / 2) {
		fprintf(stderr,
		        "Cannot reallocate tiles: 2 * %zu + 1 wraps around\n",
		        height);
		exit(EXIT_FAILURE);
	}
	const size_t newheight = 2 * height + 1, quarter = height / 2;
	bool * const new = malloc(width * newheight * sizeof(bool));
	if (new == NULL) {
		PRINT_ERR("Could not reallocate tiles");
		exit(EXIT_FAILURE);
	}
	for (size_t y = 0; y < newheight; y++) {
		for (size_t x = 0; x < width; x++)
			new[y * width + x] =
				(quarter + 1 <= y && y < quarter + 1 + height)
				&& tile[(y - quarter - 1) * width + x];
	}
	free(tile);
	tile = new;
	height = newheight;
	refy += quarter + 1;
	return y + quarter + 1;
}

static void
unexpectedchar(const uintmax_t line, const int c)
{
	fprintf(stderr, "Unexpected character on line %ju: %c\n", line, c);
}

static void
movevertically(const uintmax_t line,
               size_t * const x,
               size_t * const y,
               const int_fast8_t dir)
{
	int c;
	*y = ((*y == 0 || *y == height - 1)? doubleheight(*y) : *y) + dir;
	if ((c = getchar()) == EOF && feof(stdin)) {
		fprintf(stderr, "Input ends prematurely on line %ju\n", line);
		exit(EXIT_FAILURE);
	} else if (c == '\n') {
		fprintf(stderr, "Line %ju ends prematurely\n", line);
		exit(EXIT_FAILURE);
	} else if (dir > 0 && c == 'w') {
		*x = (*x == 0? doublewidth(*x) : *x) - 1;
	} else if (dir < 0 && c == 'e') {
		*x = (*x == width - 1? doublewidth(*x) : *x) + 1;
	} else if (!((dir < 0 && c == 'w') || (dir > 0 && c == 'e'))) {
		unexpectedchar(line, c);
		exit(EXIT_FAILURE);
	}
}

static void
parse(void)
{
	uintmax_t line = 1;
	while (!feof(stdin) && !ferror(stdin)) {
		size_t x = refx, y = refy;
		bool nonempty = false;
		int c;
		while ((c = getchar()) != EOF && c != '\n') {
			nonempty = true;
			if (c == 'w') {
				x = (x == 0? doublewidth(x) : x) - 1;
			} else if (c == 'e') {
				x = (x == width - 1? doublewidth(x) : x) + 1;
			} else if (c == 'n') {
				movevertically(line, &x, &y, -1);
			} else if (c == 's') {
				movevertically(line, &x, &y, 1);
			} else {
				unexpectedchar(line, c);
				exit(EXIT_FAILURE);
			}
		}
		if (nonempty)
			tile[y * width + x] = !tile[y * width + x];
		line++;
	}
}

static uint_fast8_t
numblackneighbors(const size_t x, const size_t y)
{
	uint_fast8_t count = 0;
	if (y > 0) {
		count += tile[(y - 1) * width + x];
		if (x + 1 < width)
			count += tile[(y - 1) * width + x + 1];
	}
	if (x > 0)
		count += tile[y * width + x - 1];
	if (x + 1 < width)
		count += tile[y * width + x + 1];
	if (y + 1 < height) {
		count += tile[(y + 1) * width + x];
		if (x > 0)
			count += tile[(y + 1) * width + x - 1];
	}
	return count;
}

static void
passday(void)
{
	for (size_t x = 0; x < width; x++) {
		if (tile[x] || tile[(height - 1) * width + x]) {
			doubleheight(0);
			break;
		}
	}
	for (size_t y = 0; y < height; y++) {
		if (tile[y * width] || tile[y * width + width - 1]) {
			doublewidth(0);
			break;
		}
	}
	bool * const new = malloc(width * height * sizeof(bool));
	if (new == NULL) {
		PRINT_ERR("Could not reallocate the tiles");
		exit(EXIT_FAILURE);
	}
	for (size_t y = 0; y < height; y++) {
		for (size_t x = 0; x < width; x++) {
			const uint_fast8_t neigh = numblackneighbors(x, y);
			new[y * width + x] = neigh == 2 || (neigh == 1 && tile[y * width + x]);
		}
	}
	free(tile);
	tile = new;
}

static uintmax_t
countblacktiles(void)
{
	uintmax_t count = 0;
	for (size_t t = 0; t < width * height; t++)
		count += tile[t];
	return count;
}

static void
freetiles(void)
{
	free(tile);
}

int
day24(void)
{
	if (atexit(freetiles) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	errno = 0;
	if ((tile = malloc(sizeof(bool))) == NULL) {
		PRINT_ERR("Could not allocate tiles");
		return EXIT_FAILURE;
	}
	tile[0] = false;
	parse();
	if (ferror(stdin)) {
		fputs("Puzzle input parsing failed\n", stderr);
		return EXIT_FAILURE;
	}
	printf("Day 0\t%ju\n", countblacktiles());
	for (int day = 0; day < 100; day++)
		passday();
	printf("Day 100\t%ju\n", countblacktiles());
	return EXIT_SUCCESS;
}

