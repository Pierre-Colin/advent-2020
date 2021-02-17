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

#define ABS(x) ((x >= 0)? x : -x)

typedef enum { EAST, NORTH, WEST, SOUTH } Direction;

static int_fast8_t
rsin(const uintmax_t n)
{
	return (n % 2) * (((n % 4 == 1) << 1) - 1);
}

static int_fast8_t
rcos(const uintmax_t n)
{
	return (n % 2 == 0) * (((n % 4 == 0) << 1) - 1);
}

int
day12(FILE * const in)
{
	intmax_t xa = 0, ya = 0, xb = 0, yb = 0, xw = 10, yw = 1, value;
	Direction dir = EAST;
	char action;
	while (fscanf(in, "%1c%ju", &action, &value) == 2) {
		const uintmax_t a = value / 90;
		intmax_t nxw;
		switch (action) {
		case 'N':
			ya += value;
			yw += value;
			break;
		case 'S':
			ya -= value;
			yw -= value;
			break;
		case 'E':
			xa += value;
			xw += value;
			break;
		case 'W':
			xa -= value;
			xw -= value;
			break;
		case 'L':
			dir = (dir + a) % 4;
			nxw = xw * rcos(a) - yw * rsin(a);
			yw = xw * rsin(a) + yw * rcos(a);
			xw = nxw;
			break;
		case 'R':
			dir = (dir - a) % 4;
			nxw = xw * rcos(a) + yw * rsin(a);
			yw = -xw * rsin(a) + yw * rcos(a);
			xw = nxw;
			break;
		case 'F':
			xa += value * rcos(dir);
			ya += value * rsin(dir);
			xb += value * xw;
			yb += value * yw;
			break;
		default:
			fprintf(stderr, "Invalid action: %c\n", action);
			return EXIT_FAILURE;
		}
		const int next = fgetc(in);
		if (next != '\n' && next != EOF) {
			fprintf(stderr, "Unexpected character: %c\n", next);
			return EXIT_FAILURE;
		}
	}
	if (!feof(in) || ferror(in)) {
		fputs("Puzzle input parsing failed\n", stderr);
		return EXIT_FAILURE;
	}
	printf("Move\t%jd\n", ABS(xa) + ABS(ya));
	printf("Waypt\t%jd\n", ABS(xb) + ABS(yb));
	return EXIT_SUCCESS;
}
