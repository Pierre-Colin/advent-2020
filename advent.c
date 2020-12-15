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

int day01(void);
int day02(void);
int day03(void);
int day04(void);
int day05(void);
int day06(void);
int day07(void);
int day08(void);
int day09(void);
int day10(void);
int day11(void);
int day12(void);
int day13(void);
int day14(void);
int day15(void);

static int (*days[])(void) = {
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
	day15
};

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
usage(const char *const cmd)
{
	const size_t ndays = sizeof(days) / sizeof(int (*)(void));
	fprintf(stderr, "usage: %s day\n", cmd);
	fprintf(stderr, "day must be an integer between 1 and %zu\n\n", ndays);
	fputs("Puzzle input must be piped into standard input.\n", stderr);
	fprintf(stderr, "Easiest way to do it is: %s day < input\n", cmd);
}

int
main(int argc, char *argv[])
{
	const size_t ndays = sizeof(days) / sizeof(int (*)(void));
	uint8_t day;
	switch (argc) {
	case 0:
		fputs("Standard library failed to initialize\n", stderr);
		return EXIT_FAILURE;
	case 2:
		day = parseday(argv[1]);
		if (1 <= day && day <= ndays) {
			return days[day - 1]();
		} else {
			fprintf(stderr,
			        "Day must be an integer between 1 and %zu\n",
			        ndays);
			return EXIT_FAILURE;
		}
	default:
		usage(argv[0]);
		return EXIT_FAILURE;
	}
}
