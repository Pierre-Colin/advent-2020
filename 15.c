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

#define PRINT_ERR(s) \
	if (errno != 0) \
		perror(s); \
	else \
		fputs(s "\n", stderr);

typedef struct {
	uintmax_t last, second;
} History;

static History *num = NULL;
static size_t sz = 0;

static bool
speak(const size_t n, const uintmax_t turn)
{
	if (sz <= n) {
		History *const new = realloc(num, (n + 1) * sizeof(History));
		if (new == NULL)
			return false;
		for (size_t i = sz; i <= n; i++)
			new[i].last = new[i].second = UINTMAX_MAX;
		num = new;
		sz = n + 1;
	}
	num[n].second = num[n].last;
	num[n].last = turn;
	return true;
}

static size_t
playturn(const uintmax_t turn, const size_t last)
{
	if (num[last].second == UINTMAX_MAX) {
		speak(0, turn);
		return 0;
	} else if (num[last].last - num[last].second > SIZE_MAX) {
		fprintf(stderr,
		        "Cannot store %ju without wraparound\n",
		        num[last].last - num[last].second);
		exit(EXIT_FAILURE);
	} else {
		const size_t n = num[last].last - num[last].second;
		if (!speak(n, turn)) {
			PRINT_ERR("Could not speak the next number");
			exit(EXIT_FAILURE);
		}
		return n;
	}
}

static void
freenum(void)
{
	free(num);
}

int
day15(void)
{
	if (atexit(freenum) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	uintmax_t turn = 0;
	size_t input, last = SIZE_MAX;
	int scanres;
	errno = 0;
	while ((scanres = scanf("%zu", &input)) == 1) {
		if (!speak(input, turn++)) {
			PRINT_ERR("Could not speak a starting number");
			return EXIT_FAILURE;
		}
		const int next = getchar();
		if (next != ',' && next != '\n' && next != EOF) {
			fprintf(stderr, "Unexpected character: %c\n", next);
			return EXIT_FAILURE;
		}
		last = input;
	}
	if (!feof(stdin)) {
		PRINT_ERR("Error occured during puzzle input parsing");
		return EXIT_FAILURE;
	}
	if (last > sz) {
		fputs("Number list was empty\n", stderr);
		return EXIT_FAILURE;
	}
	while (turn < UINTMAX_C(2020))
		last = playturn(turn++, last);
	printf("2020th\t%zu\n", last);
	while (turn < UINTMAX_C(30000000))
		last = playturn(turn++, last);
	printf("30Mth\t%zu\n", last);
	return EXIT_SUCCESS;
}

