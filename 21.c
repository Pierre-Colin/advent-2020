/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Upper bound to how many digits a given type may hold */
#define DIGITS(T) (CHAR_BIT * 10 * sizeof(T) / (9 * sizeof(char)))

struct List {
	union { const char *str; size_t i; } val;
	struct List *next;
};

typedef struct List List;

struct Food {
	List *ing;
	List *ag;
	struct Food *next;
};

typedef struct Food Food;

static Food *fdhead = NULL;

static char **ingarr = NULL;
static size_t cingarr = 0, singarr = 0;

static char **agarr = NULL;
static size_t cagarr = 0, sagarr = 0;

static bool
addtoarr(char * const restrict str,
         char *** const restrict arr,
         size_t * const restrict cap,
         size_t * const restrict sz,
         const size_t k)
{
	if (*sz == *cap) {
		*cap = (*cap > 0)? 2 * *cap : 1;
		char ** const new = realloc(*arr, *cap * sizeof(char *));
		if (new == NULL) {
			free(str);
			return false;
		}
		*arr = new;
	}
	for (size_t i = (*sz)++; i > k; i--)
		(*arr)[i] = (*arr)[i - 1];
	(*arr)[k] = str;
	return true;
}

static const char *
addstr(char * const restrict str,
       char *** const restrict arr,
       size_t * const restrict cap,
       size_t * const restrict sz)
{
	size_t a = 0, b = *sz;
	while (a + 1 < b) {
		const size_t m = (a & b) + ((a ^ b) >> 1);
		const int cmp = strcmp(str, (*arr)[m]);
		if (cmp < 0) {
			b = m;
		} else if (cmp > 0) {
			a = m;
		} else {
			free(str);
			return (*arr)[m];
		}
	}
	const int cmp = (*sz > 0)? strcmp(str, (*arr)[a]) : -1;
	if (cmp == 0) {
		free(str);
	} else {
		a += cmp > 0;
		if (!addtoarr(str, arr, cap, sz, a))
			return NULL;
	}
	return (*arr)[a];
}

static size_t
getindex(const char * const restrict str, const size_t sz, char *arr[sz])
{
	size_t a = 0, b = sz;
	while (a + 1 < b) {
		const size_t m = (a & b) + ((a ^ b) >> 1);
		const int cmp = strcmp(str, arr[m]);
		if (cmp < 0)
			b = m;
		else if (cmp > 0)
			a = m;
		else
			return m;
	}
	return a;
}

static void
freelist(List *list)
{
	while (list != NULL) {
		List * const next = list->next;
		free(list);
		list = next;
	}
}

static List *
parseing(FILE * const restrict in, bool * const restrict hasag)
{
	List *ihead = NULL, *itail = NULL;
	char *input, space;
	int result;
	while ((result = fscanf(in, "%m[a-z]%1c", &input, &space)) >= 1) {
		const char *sing = addstr(input, &ingarr, &cingarr, &singarr);
		if (sing == NULL) {
			freelist(ihead);
			return NULL;
		}
		List * const new = malloc(sizeof(List));
		if (new == NULL) {
			freelist(ihead);
			return NULL;
		}
		new->val.str = sing;
		new->next = NULL;
		if (ihead == NULL)
			ihead = new;
		else
			itail->next = new;
		if (result == 2) {
			if (space == '\n')
				return ihead;
			if (!isspace(space)) {
				freelist(ihead);
				return NULL;
			}
		} else if (result == 1 && feof(in)) {
			return ihead;
		} else {
			freelist(ihead);
			return NULL;
		}
		itail = new;
	}
	if (ferror(in)) {
		freelist(ihead);
		return NULL;
	} else if (feof(in)) {
		return ihead;
	}
	const int c = fgetc(in);
	*hasag = c == '(';
	if (c == '(' || c == EOF) {
		return ihead;
	} else {
		freelist(ihead);
		return NULL;
	}
}

static int
keepparsingag(FILE * const in)
{
	const int c = fgetc(in);
	if (c == ',')
		return 1;
	if (c == ')')
		return 0;
	return -1;
}

static bool
parseag(FILE * const restrict in, List ** const restrict agref)
{
	char *input;
	if (fscanf(in, "contains %m[a-z]", &input) < 1)
		return false;
	const char *sag = addstr(input, &agarr, &cagarr, &sagarr);
	if (sag == NULL)
		return false;
	List * const ahead = malloc(sizeof(List));
	if (ahead == NULL)
		return false;
	ahead->val.str = sag;
	ahead->next = NULL;
	List *atail = ahead;
	int result;
	while ((result = keepparsingag(in)) > 0) {
		if (fscanf(in, " %m[a-z]", &input) < 1) {
			freelist(ahead);
			return false;
		}
		if ((sag = addstr(input, &agarr, &cagarr, &sagarr)) == NULL) {
			freelist(ahead);
			return false;
		}
		List * const new = malloc(sizeof(List));
		if (new == NULL) {
			freelist(ahead);
			return false;
		}
		new->val.str = sag;
		new->next = NULL;
		atail->next = new;
		atail = new;
	}
	if (result == 0) {
		const int next = fgetc(in);
		if (next == '\n' || next == EOF) {
			*agref = ahead;
			return true;
		}
	}
	freelist(ahead);
	return false;
}

static bool
keepparsing(FILE * const in)
{
	const int c = fgetc(in);
	if (c == EOF)
		return false;
	ungetc(c, in);
	return true;
}

static void
parsepanic(const uintmax_t line,
           List * const restrict list,
           const char * const restrict err)
{
	freelist(list);
	fprintf(stderr, "Could not %s on line %ju\n", err, line);
	exit(EXIT_FAILURE);
}

static void
parse(FILE * const in)
{
	uintmax_t line = UINTMAX_C(1);
	Food *fdtail = NULL;
	while (keepparsing(in)) {
		bool hasag = false;
		List * const ing = parseing(in, &hasag);
		if (ing == NULL)
			parsepanic(line, NULL, "parse ingredients");
		Food * const new = malloc(sizeof(Food));
		if (new == NULL)
			parsepanic(line, ing, "allocade food data");
		new->ing = ing;
		new->ag = NULL;
		new->next = NULL;
		if (hasag) {
			List *ag;
			if (!parseag(in, &ag))
				parsepanic(line, ing, "parse allergen data");
			new->ag = ag;
		}
		if (fdhead == NULL)
			fdhead = new;
		else
			fdtail->next = new;
		fdtail = new;
		line++;
	}
}

static void
convert(void)
{
	for (Food *food = fdhead; food != NULL; food = food->next) {
		for (List *node = food->ing; node != NULL; node = node->next)
			node->val.i = getindex(node->val.str, singarr, ingarr);
		for (List *node = food->ag; node != NULL; node = node->next)
			node->val.i = getindex(node->val.str, sagarr, agarr);
	}
}

static void
match(bool inghasag[singarr][sagarr])
{
	for (const Food *fd = fdhead; fd != NULL; fd = fd->next) {
		bool hasing[singarr];
		memset(hasing, 0, sizeof(hasing));
		for (const List *n = fd->ing; n != NULL; n = n->next)
			hasing[n->val.i] = true;
		bool hasag[sagarr];
		memset(hasag, 0, sizeof(hasag));
		for (const List *n = fd->ag; n != NULL; n = n->next)
			hasag[n->val.i] = true;
		for (size_t ing = 0; ing < singarr; ing++) {
			for (size_t ag = 0; ag < sagarr; ag++) {
				if (hasag[ag] && !hasing[ing])
					inghasag[ing][ag] = false;
			}
		}
	}
	bool changed;
	do {
		changed = false;
		for (size_t ing = 0; ing < singarr; ing++) {
			size_t ag = 0, count = 0;
			for (size_t a = 0; a < sagarr; a++) {
				if (inghasag[ing][a]) {
					ag = a;
					count++;
				}
			}
			if (count == 1) {
				for (size_t i = 0; i < singarr; i++) {
					if (i != ing) {
						changed |= inghasag[i][ag];
						inghasag[i][ag] = false;
					}
				}
			}
		}
	} while (changed);
}

static uintmax_t
countinert(const bool inghasag[singarr][sagarr])
{
	bool inert[singarr];
	for (size_t ing = 0; ing < singarr; ing++) {
		inert[ing] = true;
		for (size_t ag = 0; ag < sagarr; ag++) {
			if (inghasag[ing][ag]) {
				inert[ing] = false;
				break;
			}
		}
	}
	uintmax_t count = 0;
	for (const Food *food = fdhead; food != NULL; food = food->next) {
		for (const List *n = food->ing; n != NULL; n = n->next)
			count += inert[n->val.i];
	}
	return count;
}

static void
printinglist(const bool inghasag[singarr][sagarr])
{
	fputs("List\t", stdout);
	for (size_t ag = 0; ag < sagarr; ag++) {
		if (ag != 0)
			putchar(',');
		for (size_t ing = 0; ing < singarr; ing++) {
			if (inghasag[ing][ag])
				fputs(ingarr[ing], stdout);
		}
	}
	putchar('\n');
}

static void
freedata(void)
{
	while (fdhead != NULL) {
		freelist(fdhead->ing);
		freelist(fdhead->ag);
		Food * const next = fdhead->next;
		free(fdhead);
		fdhead = next;
	}
	for (size_t i = 0; i < singarr; i++)
		free(ingarr[i]);
	free(ingarr);
	for (size_t i = 0; i < sagarr; i++)
		free(agarr[i]);
	free(agarr);
}

int
day21(FILE * const in)
{
	if (atexit(freedata) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	parse(in);
	convert();
	bool inghasag[singarr][sagarr];
	for (size_t ing = 0; ing < singarr; ing++) {
		for (size_t ag = 0; ag < sagarr; ag++)
			inghasag[ing][ag] = true;
	}
	match(inghasag);
	printf("Inert\t%ju\n", countinert(inghasag));
	printinglist(inghasag);
	return EXIT_FAILURE;
}
