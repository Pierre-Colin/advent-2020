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

#define PRINT_ERR(s) \
	do { \
		if (errno != 0) \
			perror(s); \
		else \
			fputs(s "\n", stderr); \
	} while (false);

#define NUM_REG "(0|[1-9][0-9]*)"

typedef enum {
	CONSTRAINTS,
	YOUR_TICKET,
	NEARBY_TICKETS
} ParseState;

typedef struct {
	uintmax_t min, max;
} Interval;

typedef struct {
	char *name;
	Interval i[2];
} FieldDef;

struct Node {
	uintmax_t *fields;
	struct Node *next;
};

typedef struct Node Node;

static FieldDef *constr = NULL;
static size_t cconstr = 0;
static size_t nconstr = 0;
static uintmax_t *yourticket = NULL;
static Node *tickets = NULL;
static regex_t defreg;

static int
getline(char **input)
{
	errno = 0;
	scanf("%*[ \t\n]");
	return scanf("%m[^\n]", input);
}

static bool
parsedef(const char *const input)
{
	regmatch_t match[6];
	if (regexec(&defreg, input, 6, match, 0) != 0) {
		fprintf(stderr, "Bad input on line %s\n", input);
		return false;
	}
	if (nconstr == cconstr) {
		cconstr = (cconstr > 0)? 2 * cconstr : 1;
		errno = 0;
		FieldDef *temp = realloc(constr, cconstr * sizeof(FieldDef));
		if (temp == NULL) {
			PRINT_ERR("Allocation failed");
			return false;
		}
		constr = temp;
	}
	errno = 0;
	char *const name = malloc((match[1].rm_eo + 1) * sizeof(char));
	if (name == NULL) {
		PRINT_ERR("Allocation failed");
		return false;
	}
	strncpy(name, input, match[1].rm_eo);
	name[match[1].rm_eo] = 0;
	errno = 0;
	int result = sscanf(input + match[2].rm_so,
	                    "%" SCNuMAX "-%" SCNuMAX
	                    " or %" SCNuMAX "-%" SCNuMAX,
	                    &constr[nconstr].i[0].min,
	                    &constr[nconstr].i[0].max,
	                    &constr[nconstr].i[1].min,
	                    &constr[nconstr].i[1].max);
	if (result == EOF) {
		fprintf(stderr,
		        "Could not match intervals on line: %s\n",
		        input);
		free(name);
		return false;
	}
	constr[nconstr++].name = name;
	return true;
}

static uintmax_t *
parsefields(const char *const input)
{
	errno = 0;
	if (nconstr == 0)
		return NULL;
	uintmax_t *const fields = malloc(nconstr * sizeof(uintmax_t));
	if (fields != NULL) {
		int off;
		const char *it = input;
		for (size_t i = 0; i < nconstr; i++) {
			errno = 0;
			if (sscanf(it, "%" SCNuMAX "%n", fields + i, &off) < 1
			    || (i != nconstr - 1 && it[off] != ',')) {
				fprintf(stderr,
				        "Could not match ticket at %zu: %s\n",
					i,
				        input);
				free(fields); 
				return NULL;
			}
			it += off + 1;
		}
		if (*(it - 1) != 0) {
			fprintf(stderr,
			        "Ticket is over %zu fields long: %s\n",
			        nconstr,
			        input);
			free(fields);
			return NULL;
		}
	}
	return fields;
}

static bool
parseyourticket(const char *const input)
{
	if (yourticket == NULL) {
		if ((yourticket = parsefields(input)) == NULL)
			return false;
	} else {
		fputs("Your ticket is ill-formed\n", stderr);
		return false;
	}
	return true;
}

static bool
isin(const uintmax_t val, const Interval i)
{
	return i.min <= val && val <= i.max;
}

static bool
satisfies(const uintmax_t val, const FieldDef constr)
{
	return isin(val, constr.i[0]) || isin(val, constr.i[1]);
}

static bool
isvalid(const uintmax_t val)
{
	for (size_t i = 0; i < nconstr; i++) {
		if (satisfies(val, constr[i]))
			return true;
	}
	return false;
}

static bool
parsenearbyticket(const char *input, Node **tail, uintmax_t *tser)
{
	bool invalid = false;
	uintmax_t *const fields = parsefields(input);
	if (fields == NULL) {
		const int temp = errno;
		fprintf(stderr, "On nearby ticket: %s\n", input);
		errno = temp;
		return false;
	}
	for (size_t i = 0; i < nconstr; i++) {
		if (!isvalid(fields[i])) {
			invalid = true;
			if (*tser > UINTMAX_MAX - fields[i]) {
				fputs("Integer overflow detected\n", stderr);
				free(fields);
				return false;
			}
			*tser += fields[i];
		}
	}
	if (invalid) {
		free(fields);
		return true;
	}
	errno = 0;
	Node *node = malloc(sizeof(Node));
	if (node == NULL) {
		free(fields);
		const int temp = errno;
		fprintf(stderr, "On nearby ticket: %s\n", input);
		errno = temp;
		return false;
	}
	node->fields = fields;
	node->next = NULL;
	if (tickets == NULL)
		tickets = node;
	else
		(*tail)->next = node;
	*tail = node;
	return true;
}

static uintmax_t
parseinput(void)
{
	Node *tail = NULL;
	ParseState state = CONSTRAINTS;
	uintmax_t tser = 0;
	char *input;
	int result;
	while ((result = getline(&input)) == 1) {
		switch (state) {
		case CONSTRAINTS:
			if (strcmp(input, "your ticket:") == 0) {
				state = YOUR_TICKET;
				break;
			}
			if (!parsedef(input)) {
				PRINT_ERR("Could not parse field definition");
				free(input);
				exit(EXIT_FAILURE);
			}
			break;
		case YOUR_TICKET:
			if (strcmp(input, "nearby tickets:") == 0) {
				state = NEARBY_TICKETS;
				break;
			}
			if (!parseyourticket(input)) {
				PRINT_ERR("Could not parse your ticket");
				free(input);
				exit(EXIT_FAILURE);
			}
			break;
		case NEARBY_TICKETS:
			if (!parsenearbyticket(input, &tail, &tser)) {
				PRINT_ERR("Could not parse nearby ticket");
				free(input);
				exit(EXIT_FAILURE);
			}
		}
		free(input);
	}
	if (ferror(stdin)) {
		PRINT_ERR("Input failed");
		exit(EXIT_FAILURE);
	} else if (result != EOF) {
		fputs("Bad input format\n", stderr);
		exit(EXIT_FAILURE);
	}
	return tser;
}

static void
sortfields(void)
{
	bool fixed[nconstr];
	for (size_t i = 0; i < nconstr; i++)
		fixed[i] = false;
	for (size_t pass = 0; pass < nconstr; pass++) {
		bool plausible[nconstr][nconstr];
		for (size_t i = 0; i < nconstr; i++) {
			for (size_t j = 0; j < nconstr; j++)
				plausible[i][j] = !fixed[i] && !fixed[j];
		}
		for (const Node *t = tickets; t != NULL; t = t->next) {
			for (size_t i = 0; i < nconstr; i++) {
				if (fixed[i])
					continue;
				for (size_t j = 0; j < nconstr; j++) {
					if (!fixed[j] && !satisfies(t->fields[i], constr[j]))
						plausible[i][j] = false;
				}
			}
		}
		for (size_t i = 0; i < nconstr; i++) {
			if (fixed[i])
				continue;
			size_t match;
			uintmax_t times = 0;
			for (size_t j = 0; j < nconstr; j++) {
				if (plausible[i][j]) {
					match = j;
					times++;
				}
			}
			if (times == 0) {
				fprintf(stderr, "%zu doesn't match\n", i);
				exit(EXIT_FAILURE);
			} else if (times == 1) {
				if (match != i) {
					const FieldDef temp = constr[i];
					constr[i] = constr[match];
					constr[match] = temp;
				}
				fixed[i] = true;
			}
		}
	}
	for (size_t i = 0; i < nconstr; i++) {
		if (!fixed[i]) {
			fputs("Not all fields could be sorted; "
			      "there are still invalid tickets\n",
			      stderr);
			exit(EXIT_FAILURE);
		}
	}
}

static uintmax_t
proddepart(void)
{
	uintmax_t p = 1;
	for (size_t i = 0; i < nconstr; i++) {
		if (strncmp(constr[i].name, "departure", 9) == 0) {
			if (p >= UINTMAX_MAX / yourticket[i]) {
				fputs("Integer overflow detected\n", stderr);
				exit(EXIT_FAILURE);
			}
			p *= yourticket[i];
		}
	}
	return p;
}

static void
freedata(void)
{
	regfree(&defreg);
	for (size_t i = 0; i < nconstr; i++)
		free(constr[i].name);
	free(constr);
	free(yourticket);
	Node *node = tickets;
	while (node != NULL) {
		Node *const temp = node->next;
		free(node->fields);
		free(node);
		node = temp;
	}
}

int
day16(void)
{
	if (atexit(freedata) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	int result = regcomp(&defreg,
	                     "^([A-Za-z ]+): " NUM_REG "-" NUM_REG " or "
	                                       NUM_REG "-" NUM_REG "$",
	                     REG_EXTENDED);
	if (result != 0) {
		char buf[128];
		regerror(result, &defreg, buf, 128);
		fprintf(stderr, "Could not compile regex: %s\n", buf);
		return EXIT_FAILURE;
	}
	printf("TSER\t%" PRIuMAX "\n", parseinput());
	sortfields();
	printf("Depart\t%" PRIuMAX "\n", proddepart());
	return EXIT_SUCCESS;
}
