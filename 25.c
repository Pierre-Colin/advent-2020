/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void
checkeof(void)
{
	int c;
	while ((c = getchar()) != EOF) {
		if (!isspace(c)) {
			fprintf(stderr, "Unexpected character: %c\n", c);
			exit(EXIT_FAILURE);
		}
	}
	if (!feof(stdin)) {
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
day25(void)
{
	uintmax_t doork, cardk;
	errno = 0;
	if (scanf("%ju%*[ \n\t]%ju", &doork, &cardk) < 2) {
		if (errno != 0)
			perror("Puzzle input parsing failed");
		else
			fputs("Bad puzzle input\n", stderr);
		return EXIT_FAILURE;
	}
	checkeof();
	printf("Key\t%ju\n", transform(cardk, findloop(doork)));
	return EXIT_SUCCESS;
}
