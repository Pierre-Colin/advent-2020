/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int
day02(void)
{
	uint8_t low, high;
	char c;
	char password[32];
	int matched;
	uint16_t numbers = 0;
	uint16_t positions = 0;
	while ((matched = scanf("%" SCNu8 "-%" SCNu8 " %c: %31s\n",
	                        &low,
	                        &high,
	                        &c,
	                        password)) == 4)
	{
		uint8_t occurences = 0;
		for (const char *it = password; *it != 0; it++) {
			if (*it == c)
				occurences++;
		}
		if (low <= occurences && occurences <= high)
			numbers++;
		const bool fmatch = password[low - 1] == c;
		const bool smatch = password[high - 1] == c;
		if ((fmatch || smatch) && !(fmatch && smatch))
			positions++;
	}
	if (matched != EOF) {
		perror("Could not parse input");
		return EXIT_FAILURE;
	}
	printf("Numbers: %" PRIu16 "\nPositions: %" PRIu16 "\n",
	       numbers,
	       positions);
	return EXIT_SUCCESS;
}
