/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_LINES 1024

static bool
hasproperty(const uint64_t *const num, const size_t n)
{
	bool found = false;
	for (size_t i = n - 25; !found && i < n - 1; i++) {
		for (size_t j = i + 1; !found && j < n; j++)
			found = num[n] == num[i] + num[j];
	}
	return found;
}

int
day09(FILE * const in)
{
	uint64_t num[MAX_LINES];
	uint64_t invalid = 0;
	size_t n = 0;
	int scanres;
	while ((scanres = fscanf(in, "%" SCNu64 "%*1[\n]", num + n)) != EOF) {
		if (scanres < 1) {
			if (errno != 0)
				perror("Failed to input");
			else
				fputs("Bad input format\n", stderr);
			return EXIT_FAILURE;
		}
		if (n >= 25 && !hasproperty(num, n) && invalid == 0) {
			invalid = num[n];
			printf("Invalid\t%" PRIu64 "\n", num[n]);
		}
		n++;
	}
	if (invalid == 0) {
		fputs("All numbers have the property\n", stderr);
		return EXIT_FAILURE;
	}
	for (size_t i = 0; i < n - 2; i++) {
		uint64_t sum = num[i], min = num[i], max = num[i];
		for (size_t j = i + 1; j < n - 1; j++) {
			if (num[j] < min)
				min = num[j];
			if (num[j] > max)
				max = num[j];
			sum += num[j];
			if (sum != invalid)
				continue;
			printf("Weak\t%" PRIu64 "\n", min + max);
			return EXIT_SUCCESS;
		}
	}
	fputs("Weakness not found\n", stderr);
	return EXIT_FAILURE;
}
