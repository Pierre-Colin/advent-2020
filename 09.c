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

static void
addnum(uintmax_t ** const num, size_t * const restrict c, const size_t n)
{
	if (n < *c)
		return;
	if (*c >= SIZE_MAX / 2) {
		fprintf(stderr, "Doubling %zu causes wraparound\n", *c);
		free(*num);
		exit(EXIT_FAILURE);
	}
	*c = *c > 0? 2 * *c : 1;
	uintmax_t * const new = realloc(*num, *c * sizeof(uintmax_t));
	if (new == NULL) {
		fprintf(stderr, "Failed to allocate array of size %zu\n", *c);
		free(*num);
		exit(EXIT_FAILURE);
	}
	*num = new;
}

int
day09(FILE * const in)
{
	uintmax_t *num = NULL, invalid = 0;
	size_t n = 0, c = 0;
	while (!feof(in) && !ferror(in)) {
		uintmax_t input;
		int next;
		if (fscanf(in, "%ju", &input) == 1) {
			addnum(&num, &c, n);
			num[n] = input;
			if (n >= 25 && !hasproperty(num, n) && invalid == 0) {
				invalid = num[n];
				printf("Invalid\t%ju\n", num[n]);
			}
			n++;
		} else if ((next = fgetc(in)) != '\n' && next != EOF) {
			fprintf(stderr, "Bad input format\n");
			free(num);
			return EXIT_FAILURE;
		}
	}
	if (!feof(in) || ferror(in)) {
		fputs("Puzzle input parsing failed\n", stderr);
		free(num);
		return EXIT_FAILURE;
	} else if (n < 25) {
		fprintf(stderr, "Need at least 25 numbers, got %zu\n", n);
		free(num);
		return EXIT_FAILURE;
	} else if (invalid == 0) {
		fputs("All numbers have the property\n", stderr);
		free(num);
		return EXIT_FAILURE;
	}
	for (size_t i = 0; i < n - 2; i++) {
		uintmax_t sum = num[i], min = num[i], max = num[i];
		for (size_t j = i + 1; j < n - 1; j++) {
			if (num[j] < min)
				min = num[j];
			if (num[j] > max)
				max = num[j];
			sum += num[j];
			if (sum != invalid)
				continue;
			printf("Weak\t%ju\n", min + max);
			free(num);
			return EXIT_SUCCESS;
		}
	}
	fputs("Weakness not found\n", stderr);
	free(num);
	return EXIT_FAILURE;
}
