/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void
checkeof(FILE * const in)
{
	int c;
	while ((c = fgetc(in)) != EOF) {
		if (!isspace(c)) {
			fprintf(stderr, "Unexpected character: %c\n", c);
			exit(EXIT_FAILURE);
		}
	}
	if (!feof(in) || ferror(in)) {
		fputs("Puzzle input parsing failed\n", stderr);
		exit(EXIT_FAILURE);
	}
}

static uintmax_t
transform(const uintmax_t subject, uintmax_t loop)
{
	uintmax_t value = 1;
	while (loop--)
		value = (value * subject) % 20201227;
	return value;
}

static uintmax_t
findloop(const uintmax_t key)
{
	uintmax_t loop = 0, val = 1;
	while (val != key) {
		loop++;
		val = (7 * val) % 20201227;
	}
	return loop;
}

int
day25(FILE * const in)
{
	uintmax_t doork, cardk;
	if (fscanf(in, "%ju%*[ \n\t]%ju", &doork, &cardk) < 2) {
		fputs("Puzzle input parsing failed\n", stderr);
		return EXIT_FAILURE;
	}
	checkeof(in);
	printf("Key\t%ju\n", transform(cardk, findloop(doork)));
	return EXIT_SUCCESS;
}
