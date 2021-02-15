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

#define PRINT_ERR(s) \
	if (errno != 0) \
		perror(s); \
	else \
		fputs(s "\n", stderr);

static uint_fast32_t
playturn(uint_fast32_t num[UINT32_C(30000000)],
         const uint_fast32_t turn,
         const uint_fast32_t last)
{
	const uint_fast32_t temp = num[last];
	num[last] = turn;
	return (temp <= turn) * (turn - temp);
}

int
day15(FILE * const in)
{
	uint_fast32_t turn = 0, last = UINT_FAST32_MAX, input;
	errno = 0;
	uint_fast32_t *num = malloc(UINT32_C(30000000) * sizeof(uint_fast32_t));
	if (num == NULL) {
		PRINT_ERR("Could not allocate the number history");
		return EXIT_FAILURE;
	}
	for (uint_fast32_t i = 0; i < UINT32_C(30000000); i++)
		num[i] = UINT_FAST32_MAX;
	while (fscanf(in, "%" SCNuFAST32, &input) == 1) {
		if (last != UINT_FAST32_MAX)
			num[last] = turn;
		last = input;
		const int next = fgetc(in);
		if (next != ',' && next != '\n' && next != EOF) {
			fprintf(stderr, "Unexpected character: %c\n", next);
			free(num);
			return EXIT_FAILURE;
		}
		turn++;
	}
	if (!feof(in)) {
		PRINT_ERR("Error occured during puzzle input parsing");
		free(num);
		return EXIT_FAILURE;
	}
	if (last > UINT32_C(30000000)) {
		fputs("Number list was empty\n", stderr);
		free(num);
		return EXIT_FAILURE;
	}
	while (turn < UINT32_C(2020))
		last = playturn(num, turn++, last);
	printf("2020th\t%" PRIuFAST32 "\n", last);
	while (turn < UINT32_C(30000000))
		last = playturn(num, turn++, last);
	printf("30Mth\t%" PRIuFAST32 "\n", last);
	free(num);
	return EXIT_SUCCESS;
}
