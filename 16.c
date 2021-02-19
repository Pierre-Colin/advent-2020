/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
static size_t cconstr = 0, nconstr = 0;
static uintmax_t *yourticket = NULL;
static Node *tickets = NULL;

static int
getline(FILE * const restrict in, char ** const restrict input)
{
	fscanf(in, "%*[ \t\n]");
	return fscanf(in, "%m[^\n]", input);
}

static bool
parsedef(const char * const input)
{
	if (nconstr == cconstr) {
		cconstr = (cconstr > 0)? 2 * cconstr : 1;
		FieldDef *temp = realloc(constr, cconstr * sizeof(FieldDef));
		if (temp == NULL) {
			fputs("Could not allocate field definition\n", stderr);
			return false;
		}
		constr = temp;
	}
	constr[nconstr].name = NULL;
	int n;
	if (sscanf(input,
	           "%m[A-Za-z ]: %ju-%ju or %ju-%ju%n",
	           &constr[nconstr].name,
	           &constr[nconstr].i[0].min,
	           &constr[nconstr].i[0].max,
	           &constr[nconstr].i[1].min,
	           &constr[nconstr].i[1].max,
	           &n) != 5
	    || input[n] != 0) {
		free(constr[nconstr].name);
		return false;
	}
	nconstr++;
	return true;
}

static uintmax_t *
parsefields(const char * const input)
{
	if (nconstr == 0)
		return NULL;
	uintmax_t * const fields = malloc(nconstr * sizeof(uintmax_t));
	if (fields == NULL)
		return NULL;
	int off;
	const char *it = input;
	for (size_t i = 0; i < nconstr; i++) {
		if (sscanf(it, "%ju%n", fields + i, &off) < 1
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
	return fields;
}

static bool
parseyourticket(const char * const input)
{
	if (yourticket == NULL) {
		return (yourticket = parsefields(input)) != NULL;
	} else {
		fputs("Your ticket is ill-formed\n", stderr);
		return false;
	}
}

static bool
isin(const uintmax_t val, const Interval * const i)
{
	return i->min <= val && val <= i->max;
}

static bool
satisfies(const uintmax_t val, const FieldDef * const constr)
{
	return isin(val, &constr->i[0]) || isin(val, &constr->i[1]);
}

static bool
isvalid(const uintmax_t val)
{
	for (size_t i = 0; i < nconstr; i++) {
		if (satisfies(val, constr + i))
			return true;
	}
	return false;
}

static bool
parsenearbyticket(const char * const restrict input,
                  Node ** const restrict tail,
                  uintmax_t * const restrict tser)
{
	uintmax_t * const fields = parsefields(input);
	if (fields == NULL)
		return false;
	bool invalid = false;
	for (size_t i = 0; i < nconstr; i++) {
		if (!isvalid(fields[i])) {
			invalid = true;
			if (*tser > UINTMAX_MAX - fields[i]) {
				fputs("Integer wraparound detected\n", stderr);
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
	Node * const node = malloc(sizeof(Node));
	if (node == NULL) {
		free(fields);
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

static ParseState
parsecase(const ParseState state,
          char * const restrict input,
          const char * const restrict nextstr,
          bool (*parsefunc)(const char*),
          const char * const restrict parsing)
{
	if (strcmp(input, nextstr) == 0)
		return state + 1;
	if (!parsefunc(input)) {
		fprintf(stderr, "Could not parse %s: %s\n", parsing, input);
		free(input);
		exit(EXIT_FAILURE);
	}
	return state;
}

static uintmax_t
parseinput(FILE * const in)
{
	Node *tail = NULL;
	ParseState state = CONSTRAINTS;
	uintmax_t tser = 0;
	char *input;
	while (getline(in, &input) == 1) {
		switch (state) {
		case CONSTRAINTS:
			state = parsecase(state,
			                  input,
			                  "your ticket:",
			                  parsedef,
			                  "ticket field definition");
			break;
		case YOUR_TICKET:
			state = parsecase(state,
			                  input,
			                  "nearby tickets:",
			                  parseyourticket,
			                  "your ticket");
			break;
		case NEARBY_TICKETS:
			if (!parsenearbyticket(input, &tail, &tser)) {
				fprintf(stderr,
				        "Could not parse nearby ticket: %s\n",
				        input);
				free(input);
				exit(EXIT_FAILURE);
			}
		}
		free(input);
	}
	if (!feof(in) || ferror(in)) {
		fputs("Error occured while parsing puzzle input\n", stderr);
		exit(EXIT_FAILURE);
	}
	return tser;
}

static void
eliminateimplausible(const uintmax_t tikfields[const restrict nconstr],
                     const bool fixed[const restrict nconstr],
                     bool plausible[nconstr][nconstr])
{
	for (size_t i = 0; i < nconstr; i++) {
		if (fixed[i])
			continue;
		for (size_t j = 0; j < nconstr; j++) {
			if (!fixed[j] && !satisfies(tikfields[i], constr + j))
				plausible[i][j] = false;
		}
	}
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
		for (const Node *tik = tickets; tik != NULL; tik = tik->next)
			eliminateimplausible(tik->fields, fixed, plausible);
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
				fputs("Integer wraparound detected\n", stderr);
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
	for (size_t i = 0; i < nconstr; i++)
		free(constr[i].name);
	free(constr);
	free(yourticket);
	while (tickets != NULL) {
		Node * const next = tickets->next;
		free(tickets->fields);
		free(tickets);
		tickets = next;
	}
}

int
day16(FILE * const in)
{
	if (atexit(freedata) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	printf("TSER\t%ju\n", parseinput(in));
	sortfields();
	printf("Depart\t%ju\n", proddepart());
	return EXIT_SUCCESS;
}
