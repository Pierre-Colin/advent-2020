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

typedef enum {
	EAST,
	NORTH,
	WEST,
	SOUTH
} Direction;

static int8_t
rsin(uint64_t n)
{
	if (n % 2 == 0)
		return 0;
	if (n % 4 == 1)
		return 1;
	return -1;
}

static int8_t
rcos(uint64_t n)
{
	if (n % 2 == 1)
		return 0;
	if (n % 4 == 0)
		return 1;
	return -1;
}

int
day12(FILE * const in)
{
	int64_t xa = 0, ya = 0;
	int64_t xb = 0, yb = 0;
	int64_t xw = 10, yw = 1;
	Direction dir = EAST;
	char action;
	uint16_t value;
	int scanres;
	while ((scanres = fscanf(in, "%c%" SCNu16 "%*1[\n]", &action, &value))
	       != EOF) {
		if (scanres < 2) {
			if (errno != 0)
				perror("Input failed");
			else
				fputs("Bad input format\n", stderr);
			return EXIT_FAILURE;
		}
		const uint64_t a = value / 90;
		int64_t nxw;
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
	}
	printf("Move\t%" PRIi64 "\n", ABS(xa) + ABS(ya));
	printf("Waypt\t%" PRIi64 "\n", ABS(xb) + ABS(yb));
	return EXIT_SUCCESS;
}
