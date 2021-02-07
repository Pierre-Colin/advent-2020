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

int
day02(FILE * const in)
{
	size_t low, high;
	char c, *pass;
	uintmax_t numbers = 0, positions = 0;
	while (fscanf(in, "%zu-%zu %c: %m[a-z]", &low, &high, &c, &pass) == 4) {
		size_t occurences = 0;
		for (const char *it = pass; *it != 0; it++) {
			if (*it == c)
				occurences++;
		}
		if (low <= occurences && occurences <= high)
			numbers++;
		const bool fmatch = pass[low - 1] == c;
		const bool smatch = pass[high - 1] == c;
		free(pass);
		if ((fmatch || smatch) && !(fmatch && smatch))
			positions++;
		if (fgetc(in) != '\n')
			break;
	}
	if (!feof(in)) {
		perror("Could not parse input");
		return EXIT_FAILURE;
	}
	printf("Numbers: %ju\nPositions: %ju\n", numbers, positions);
	return EXIT_SUCCESS;
}
