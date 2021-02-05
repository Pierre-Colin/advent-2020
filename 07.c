/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <errno.h>
#include <inttypes.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE_MATCH(r) ((size_t) (r.rm_eo - r.rm_so + 1))
#define MAX_RULES 1024

typedef union {
	char *str;
	size_t id;
} ContainUnion;

struct ContainNode {
	uint8_t quantity;
	ContainUnion u;
	struct ContainNode *next;
};

typedef struct ContainNode ContainNode;

typedef struct {
	char *container;
	ContainNode *contains;
} Rule;

static Rule rules[MAX_RULES];
static size_t nrules = 0;
static bool converted = false;
static regex_t listpattern, inputpattern;

static void
addrule(const Rule rule)
{
	if (nrules >= MAX_RULES) {
		fputs("Maximum rule number reached\n", stderr);
		exit(EXIT_FAILURE);
	}
	rules[nrules] = rule;
	for (size_t i = nrules++; i > 0; i--) {
		if (strcmp(rules[i].container, rules[i - 1].container) > 0)
			break;
		Rule temp = rules[i];
		rules[i] = rules[i - 1];
		rules[i - 1] = temp;
	}
}

static size_t
getrule(const char *const container)
{
	size_t a = 0, b = nrules - 1;
	do {
		const size_t m = (a / 2 + b / 2) + (a % 2 + b % 2 == 2);
		const int cmp = strcmp(rules[m].container, container);
		if (cmp < 0)
			a = m;
		else if (cmp > 0)
			b = m;
		else
			return m;
	} while (a + 1 < b);
	if (strcmp(rules[a].container, container) == 0)
		return a;
	if (strcmp(rules[b].container, container) == 0)
		return b;
	return nrules;
}

static void
freecontainlist(ContainNode *node)
{
	while (node != NULL) {
		if (!converted)
			free(node->u.str);
		ContainNode *const temp = node->next;
		free(node);
		node = temp;
	}
}

static char *
clonefromto(const char *const string, const regmatch_t bounds)
{
	char *new = malloc(SIZE_MATCH(bounds) * sizeof(char));
	if (new != NULL) {
		memcpy(new,
		       string + bounds.rm_so,
		       bounds.rm_eo - bounds.rm_so);
		new[bounds.rm_eo - bounds.rm_so] = 0;
	}
	return new;
}

static ContainNode *
makelist(const char *str)
{
	errno = 0;
	if (strcmp(str, "no other bags.") == 0)
		return NULL;
	ContainNode *head = NULL;
	ContainNode *tail = NULL;
	while (str[0] != 0) {
		regmatch_t match[5];
		if (regexec(&listpattern, str, 5, match, 0) == REG_NOMATCH) {
			errno = EINVAL;
			freecontainlist(head);
			return NULL;
		}
		uint8_t quantity = 0;
		for (regoff_t i = match[1].rm_so; i < match[1].rm_eo; i++) {
			const uint8_t new = 10 * quantity + str[i] - '0';
			if (new < quantity) {
				errno = EINVAL;
				freecontainlist(head);
				return NULL;
			}
			quantity = new;
		}
		char *const bag = clonefromto(str, match[2]);
		ContainNode *new = malloc(sizeof(ContainNode));
		if (new == NULL) {
			free(bag);
			freecontainlist(head);
			return NULL;
		}
		new->quantity = quantity;
		new->u.str = bag;
		new->next = NULL;
		if (head == NULL)
			head = new;
		else
			tail->next = new;
		tail = new;
		str += match[4].rm_eo;
	}
	return head;
}

static bool
convertrules(void)
{
	bool success = true;
	converted = true;
	for (size_t i = 0; i < nrules; i++) {
		ContainNode *node;
		for (node = rules[i].contains; node; node = node->next) {
			const size_t id = getrule(node->u.str);
			if (id == nrules)
				success = false;
			free(node->u.str);
			node->u.id = id;
		}
	}
	return success;
}

static void
freedata(void)
{
	for (size_t i = 0; i < nrules; i++) {
		free(rules[i].container);
		freecontainlist(rules[i].contains);
	}
	regfree(&listpattern);
	regfree(&inputpattern);
}

static bool
hasbag(const Rule *const rule, const char *const name, const size_t calls)
{
	if (strcmp(rule->container, name) == 0)
		return true;
	if (calls == 0) {
		return false;
		exit(EXIT_FAILURE);
	}
	for (ContainNode *node = rule->contains; node; node = node->next) {
		if (hasbag(rules + node->u.id, name, calls - 1))
			return true;
	}
	return false;
}

static uint64_t
countbags(const size_t id)
{
	uint64_t count = 0;
	for (ContainNode *node = rules[id].contains; node; node = node->next)
		count += node->quantity * (1 + countbags(node->u.id));
	return count;
}

static void
tryregcomp(regex_t *const pattern, const char *const regex)
{
	int result = regcomp(pattern, regex, REG_EXTENDED);
	if (result != 0) {
		char buf[128];
		regerror(result, pattern, buf, sizeof(buf) / sizeof(char));
		fprintf(stderr, "Could not compile regex: %s\n", buf);
		exit(EXIT_FAILURE);
	}
}

int
day07(FILE * const in)
{
	char *input;
	if (atexit(freedata) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	tryregcomp(&inputpattern, "^([a-z ]+) bags contain ([0-9a-z ,]+)\\.$");
	tryregcomp(&listpattern, "^([0-9]+) (([a-z ])+) bags?(, |\\.)");
	int scanres;
	while ((scanres = fscanf(in, "%m[0-9a-z ,.]%*1[\n]", &input)) != EOF) {
		if (scanres < 1) {
			fputs("Bad input format\n", stderr);
			return EXIT_FAILURE;
		}
		regmatch_t regmatch[3];
		if (regexec(&inputpattern, input, 3, regmatch, 0) != 0) {
			free(input);
			fputs("Bad input format\n", stderr);
			return EXIT_FAILURE;
		}
		char *const new = clonefromto(input, regmatch[1]);
		if (new == NULL) {
			free(input);
			perror("String copy failed");
			return EXIT_FAILURE;
		}
		Rule rule;
		rule.container = new;
		rule.contains = makelist(input + regmatch[2].rm_so);
		if (errno != 0) {
			free(input);
			perror("Could not allocate list");
			return EXIT_FAILURE;
		}
		addrule(rule);
		free(input);
	}
	if (!convertrules()) {
		fputs("A bag contains a nonexisting bag\n", stderr);
		return EXIT_FAILURE;
	}
	uint16_t nbags = 0;
	for (size_t i = 0; i < nrules; i++) {
		if (hasbag(rules + i, "shiny gold", nrules))
			nbags++;
	}
	printf("w/ SGB\t%" PRIu16 "\n", nbags - 1);
	const size_t id = getrule("shiny gold");
	if (id == nrules) {
		fputs("Shiny gold bag not found\n", stderr);
		return EXIT_FAILURE;
	}
	printf("In SGB\t%" PRIu64 "\n", countbags(id));
	return EXIT_SUCCESS;
}
