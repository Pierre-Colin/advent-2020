/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	const char *name;
	const char *regex;
	regex_t pattern;
} Field;

static Field fielddefs[] = {
	{ .name = "byr", .regex = "^19([2-9][0-9])|200[0-2]$" },
	{ .name = "iyr", .regex = "^20(1[0-9]|20)$" },
	{ .name = "eyr", .regex = "^20(2[0-9]|30)$" },
	{ .name = "hgt",
	  .regex = "^(1([5-8][0-9]|9[0-3])cm|(59|6[0-9]|7[0-6])in)$" },
	{ .name = "hcl", .regex = "^#[0-9a-f]{6}$" },
	{ .name = "ecl", .regex = "^(amb|blu|brn|gry|grn|hzl|oth)$" },
	{ .name = "pid", .regex = "^[0-9]{9}$" },
	{ .name = "cid", .regex = "" }
};

static void
freefields(void)
{
	for (size_t f = 0; f < sizeof(fielddefs) / sizeof(Field); f++)
		regfree(&fielddefs[f].pattern);
}

static void
checkpassport(uint8_t *fields, bool error, uint64_t *present, uint64_t *valid)
{
	if ((*fields & 0x7f) == 0x7f) {
		(*present)++;
		if (!error)
			(*valid)++;
	}
}

static void
tryfields(const char *name, const char *value, uint8_t *fields, bool *error)
{
	for (uint8_t f = 0; f < sizeof(fielddefs) / sizeof(Field); f++) {
		if (strcmp(name, fielddefs[f].name) == 0) {
			if (regexec(&fielddefs[f].pattern, value, 0, NULL, 0)
			    == REG_NOMATCH || (*fields & (1u << f)))
				*error = true;
			*fields |= 1 << f;
			break;
		}
	}
}

int
day04(void)
{
	if (atexit(freefields) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	for (size_t f = 0; f < sizeof(fielddefs) / sizeof(Field); f++) {
		const int err = regcomp(&fielddefs[f].pattern,
		                        fielddefs[f].regex,
		                        REG_EXTENDED | REG_NOSUB); 
		if (err != 0) {
			char errbuf[128];
			regerror(err, &fielddefs[f].pattern, errbuf, 128);
			fprintf(stderr,
			        "Could not compile regex: %s\n",
			        errbuf);
		}
	}
	char field[4];
	uint8_t fields = 0;
	bool error = false;
	int scanres;
	uint64_t present = 0;
	uint64_t valid = 0;
	char *value;
	while ((scanres = scanf("%3[a-z]:%m[#0-9a-z]", field, &value)) != EOF) {
		if (scanres != 2) {
			if (errno != 0)
				perror("Failed to read from standard input");
			else
				fputs("Bad input format\n", stderr);
			return EXIT_FAILURE;
		}
		tryfields(field, value, &fields, &error);
		free(value);
		int end;
		size_t lines = 0;
		do {
			end = getchar();
			if (end == '\n')
				lines++;
		} while (isspace(end));
		ungetc(end, stdin);
		if (lines > 1) {
			checkpassport(&fields, error, &present, &valid);
			fields = 0;
			error = false;
		}
	}
	checkpassport(&fields, error, &present, &valid);
	printf("Present\t%" PRIu64 "\n", present);
	printf("Valid\t%" PRIu64 "\n", valid);
	return EXIT_SUCCESS;
}

