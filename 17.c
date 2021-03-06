/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ACTIVE_C(a, x, y, z, w, xs, ys, zs) \
	a[(w) * (xs) * (ys) * (zs) + (z) * (xs) * (ys) + (y) * (xs) + (x)]

#define ACTIVE(a, x, y, z, w) ACTIVE_C(a, x, y, z, w, xsize, ysize, zsize)

static bool *pattern = NULL;
static size_t width = 0, height = 0;

static bool *space = NULL;
static size_t xsize = 0, ysize = 0, zsize = 0, wsize = 0;

/* During the parsing phase, `xsize` keeps track of the buffer size */
static void
bufappend(const size_t x, const bool val)
{
	const size_t i = height * width + x;
	if (i >= xsize) {
		xsize = (xsize > 0)? 2 * xsize : 1;
		bool * const temp = realloc(pattern, xsize * sizeof(bool));
		if (temp == NULL) {
			fputs("Could not allocate new pattern\n", stderr);
			exit(EXIT_FAILURE);
		}
		pattern = temp;
	}
	pattern[i] = val;
}

static void
parseinput(FILE * const in)
{
	size_t x = 0;
	int c;
	while ((c = fgetc(in)) != EOF) {
		switch (c) {
		case '\n':
			if (width > 0 && x != width) {
				fprintf(stderr,
				        "Line %zu doesn't have length %zu\n",
				        height,
				        width);
				exit(EXIT_FAILURE);
			} else if (width == 0) {
				width = x;
			}
			height++;
			x = 0;
			break;
		case '.':
		case '#':
			if (width != 0 && x >= width) {
				fprintf(stderr,
				        "Line %zu doesn't have length %zu\n",
				        height,
				        width);
				exit(EXIT_FAILURE);
			}
			bufappend(x++, c == '#');
			break;
		default:
			fprintf(stderr, "Invalid character: %c\n", c);
			exit(EXIT_FAILURE);
		}
	}
	if (!feof(in) || ferror(in)) {
		fputs("Error occured while parsing puzzle input\n", stderr);
		exit(EXIT_FAILURE);
	}
	if (width > 0 && x == width) {
		height++;
	} else if (x > 0) {
		fprintf(stderr, "Last line doesn't have length %zu\n", width);
		exit(EXIT_FAILURE);
	}
}

static void
copypattern(void)
{
	bool * const new = malloc(width * height * sizeof(bool));
	if (new == NULL) {
		fputs("Could not allocate pattern copy\n", stderr);
		exit(EXIT_FAILURE);
	}
	xsize = width;
	ysize = height;
	zsize = wsize = 1;
	for (size_t y = 0; y < height; y++) {
		for (size_t x = 0; x < width; x++)
			ACTIVE(new, x, y, 0, 0) = pattern[y * width + x];
	}
	space = new;
}

static bool
isborder(const size_t x, const size_t y, const size_t z, const size_t w)
{
	return x == 0 || x == xsize - 1 ||
	       y == 0 || y == ysize - 1 ||
	       z == 0 || z == zsize - 1 ||
	       w == 0 || w == wsize - 1;
}

#define SCAN(v) size_t v = 0; !stop && v < v##size; v++

static void
scanborders(bool *incx, bool *incy, bool *incz, bool *incw)
{
	bool stop = false;
	for (SCAN(w)) for (SCAN(z)) for (SCAN(y)) for (SCAN(x)) {
		if (!isborder(x, y, z, w) || !ACTIVE(space, x, y, z, w))
			continue;
		if (x == 0 || x == xsize - 1)
			*incx = true;
		if (y == 0 || y == ysize - 1)
			*incy = true;
		if (z == 0 || z == zsize - 1)
			*incz = true;
		if (w == 0 || w == wsize - 1)
			*incw = true;
		stop = *incx && *incy && *incz && *incw;
	}
}

#define INNER(v) size_t v = 1; v + 1 < new##v; v++

static void
resizespace(const bool incx, const bool incy, const bool incz, const bool incw)
{
	const size_t newx = xsize + 2 * incx;
	const size_t newy = ysize + 2 * incy;
	const size_t newz = zsize + 2 * incz;
	const size_t neww = wsize + 2 * incw;
	bool *const new = calloc(newx * newy * newz * neww, sizeof(bool));
	if (new == NULL) {
		fputs("Could not reallocate space\n", stderr);
		exit(EXIT_FAILURE);
	}
	for (INNER(w)) for (INNER(z)) for (INNER(y)) for (INNER(x)) {
		ACTIVE_C(new, x, y, z, w, newx, newy, newz)
			= ACTIVE(space, x - incx, y - incy, z - incz, w - incw);
	}
	free(space);
	space = new;
	xsize = newx;
	ysize = newy;
	zsize = newz;
	wsize = neww;
}

#define NEIGH(v) \
	size_t v = (v##v > 0)? v##v - 1 : 0; v < v##size && v <= v##v + 1; v++

static uint_fast8_t
countneighbors(size_t xx, size_t yy, size_t zz, size_t ww)
{
	uint_fast8_t total = 0;
	for (NEIGH(w)) for (NEIGH(z)) for (NEIGH(y)) for (NEIGH(x)) {
		if (!(x == xx && y == yy && z == zz && w == ww)
		    && ACTIVE(space, x, y, z, w))
			total++;
	}
	return total;
}

static bool
willbeactive(const size_t x, const size_t y, const size_t z, const size_t w)
{
	const uint_fast8_t neighbors = countneighbors(x, y, z, w);
	if (neighbors == 2)
		return ACTIVE(space, x, y, z, w);
	return neighbors == 3;
}

static void
runcycle(void)
{
	bool incx = false, incy = false, incz = false, incw = false;
	scanborders(&incx, &incy, &incz, &incw);
	if (incx || incy || incz || incw)
		resizespace(incx, incy, incz, incw);
	bool * const new = malloc(xsize * ysize * zsize * wsize * sizeof(bool));
	if (new == NULL) {
		fputs("Could not allocate new space\n", stderr);
		exit(EXIT_FAILURE);
	}
	for (size_t w = 0; w < wsize; w++) {
		for (size_t z = 0; z < zsize; z++) {
			for (size_t y = 0; y < ysize; y++) {
				for (size_t x = 0; x < xsize; x++)
					ACTIVE(new, x, y, z, w) =
						willbeactive(x, y, z, w);
			}
		}
	}
	free(space);
	space = new;
}

static void
cleanw(void)
{
	if (wsize == 1)
		return;
	for (size_t z = 0; z < zsize; z++) {
		for (size_t y = 0; y < ysize; y++) {
			for (size_t x = 0; x < xsize; x++) {
				ACTIVE(space, x, y, z, 0) = false;
				ACTIVE(space, x, y, z, wsize - 1) = false;
			}
		}
	}
}

static uintmax_t
countactive(void)
{
	uintmax_t count = 0;
	for (size_t w = 0; w < wsize; w++) {
		for (size_t z = 0; z < zsize; z++) {
			for (size_t y = 0; y < ysize; y++) {
				for (size_t x = 0; x < xsize; x++)
					count += ACTIVE(space, x, y, z, w);
			}
		}
	}
	return count;
}

static void
freespace(void)
{
	free(pattern);
	free(space);
}

int
day17(FILE * const in)
{
	if (atexit(freespace) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	parseinput(in);
	copypattern();
	for (uint_fast8_t cycle = 1; cycle <= 6; cycle++) {
		runcycle();
		cleanw();
	}
	printf("3D\t%ju\n", countactive());
	free(space);
	space = NULL;
	copypattern();
	for (uint_fast8_t cycle = 1; cycle <= 6; cycle++)
		runcycle();
	printf("4D\t%ju\n", countactive());
	return EXIT_SUCCESS;
}
