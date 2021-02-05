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
#include <string.h>
#include <time.h>

int day01(FILE *);
int day02(FILE *);
int day03(FILE *);
int day04(FILE *);
int day05(FILE *);
int day06(FILE *);
int day07(FILE *);
int day08(FILE *);
int day09(FILE *);
int day10(FILE *);
int day11(FILE *);
int day12(FILE *);
int day13(FILE *);
int day14(FILE *);
int day15(FILE *);
int day16(FILE *);
int day17(FILE *);
int day18(FILE *);
int day19(FILE *);
int day20(FILE *);
int day21(FILE *);
int day22(FILE *);
int day23(FILE *);
int day24(FILE *);
int day25(FILE *);

static int (*days[])(FILE *) = {
	day01,
	day02,
	day03,
	day04,
	day05,
	day06,
	day07,
	day08,
	day09,
	day10,
	day11,
	day12,
	day13,
	day14,
	day15,
	day16,
	day17,
	day18,
	day19,
	day20,
	day21,
	day22,
	day23,
	day24,
	day25
};

static FILE *file = NULL;

static uint8_t
parseday(const char *s)
{
	uint8_t acc = 0;
	while (*s != 0) {
		if (!isdigit(*s))
			return 0;
		acc = 10 * acc + *s++ - '0';
		if (acc > 25)
			return 0;
	}
	return acc;
}

static void
closefile(void)
{
	if (file != NULL)
		fclose(file);
}

static void
runall(void)
{
	const size_t ndays = sizeof(days) / sizeof(int (*)(FILE *));
	clock_t total = 0, chrono[ndays];
	if (atexit(closefile) != 0)
		fputs("Call to `atexit` failed;"
		      "last file may not be closed properly\n",
		      stderr);
	for (size_t d = 0; d < ndays; d++) {
		fprintf(stderr, "\tDay %zu\n", d + 1);
		char fname[16];
		sprintf(fname, "input-%zu", d + 1);
		errno = 0;
		if ((file = fopen(fname, "r")) == NULL) {
			if (errno != 0)
				perror(fname);
			else
				fprintf(stderr, "Could not open %s\n", fname);
			chrono[d] = 0;
			continue;
		}
		const clock_t begin = clock();
		days[d](file);
		chrono[d] = clock() - begin;
		fclose(file);
		total += chrono[d];
		fputc('\n', stderr);
	}
	file = NULL;
	fputs("Summary\nDay\tTicks\tms\t%\n", stderr);
	for (size_t d = 0; d < ndays; d++)
		fprintf(stderr,
		        "%zu\t%7.0lf\t%4.2lf\t%2.3lf\n",
		        d + 1,
		        (double) chrono[d],
		        1000. * (double) chrono[d] / (double) CLOCKS_PER_SEC,
		        100. * (double) chrono[d] / (double) total);
	fprintf(stderr,
	        "Total\t%7.0lf\t%4.2lf\t100\n\n",
	        (double) total,
	        1000. * (double) total / (double) CLOCKS_PER_SEC);
}

static void
usage(const char *const cmd)
{
	const size_t ndays = sizeof(days) / sizeof(int (*)(FILE *));
	fprintf(stderr, "usage: %s day\n", cmd);
	fprintf(stderr, "day must be an integer between 1 and %zu\n\n", ndays);
	fputs("Puzzle input must be piped into standard input.\n", stderr);
	fprintf(stderr, "Easiest way to do it is: %s day < input\n", cmd);
}

int
main(int argc, char *argv[])
{
	const size_t ndays = sizeof(days) / sizeof(int (*)(FILE *));
	uint8_t day;
	switch (argc) {
	case 0:
		fputs("Standard library failed to initialize\n", stderr);
		return EXIT_FAILURE;
	case 2:
		if (strcmp(argv[1], "all") == 0) {
			runall();
			return EXIT_SUCCESS;
		} else {
			day = parseday(argv[1]);
			if (1 <= day && day <= ndays) {
				return days[day - 1](stdin);
			} else {
				fprintf(stderr,
					"Day must be an integer between 1 and %zu\n",
					ndays);
				return EXIT_FAILURE;
			}
		}
	default:
		usage(argv[0]);
		return EXIT_FAILURE;
	}
}
