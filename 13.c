/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct Node {
	uintmax_t rem, div;
	struct Node *next;
};

typedef struct Node Node;

static Node *head = NULL;

static uintmax_t
xgcd(const intmax_t a,
     const intmax_t b,
     intmax_t * const restrict x,
     intmax_t * const restrict y)
{
	intmax_t m[2][2] = { { 1, 0 }, { 0, 1 } };
	uintmax_t r[2] = { a, b };
	while (r[1] > 0) {
		const intmax_t q = r[0] / r[1];
		const uintmax_t rr = r[0] - q * r[1];
		const intmax_t aa = m[0][0] - q * m[1][0];
		const intmax_t bb = m[0][1] - q * m[1][1];
		m[0][0] = m[1][0];
		m[0][1] = m[1][1];
		m[1][0] = aa;
		m[1][1] = bb;
		r[0] = r[1];
		r[1] = rr;
	}
	*x = m[0][0];
	*y = m[0][1];
	return r[0];
}

static uintmax_t
posmod(intmax_t x, const uintmax_t n)
{
	if (x < 0)
		x += n * (-x / n + 1);
	return x % n;
}

static uintmax_t
mul2mod(uintmax_t x, uintmax_t y, const uintmax_t n)
{
	x %= n;
	y %= n;
	uintmax_t r = 0;
	while (y > 0) {
		if (y % 2 == 1)
			r = (r + x) % n;
		x = (2 * x) % n;
		y /= 2;
	}
	return r;
}

static uintmax_t
mulmod(uintmax_t x, uintmax_t y, uintmax_t z, const uintmax_t n)
{
	return mul2mod(mul2mod(x, y, n), z, n);
}

static uintmax_t
addmod(const uintmax_t x, const uintmax_t y, const uintmax_t n)
{
	return ((x % n) + (y % n)) % n;
}

static void
retryinput(FILE * const restrict in, uintmax_t * const restrict rem)
{
	if (ferror(in)) {
		fputs("Internal error occured during input parsing\n", stderr);
		exit(EXIT_FAILURE);
	}
	char c[2];
	const size_t res = fread(c, sizeof(char), 2, in);
	if (res < 2 && (!feof(in) || ferror(in))) {
		fputs("Error occured while reading puzzle input\n", stderr);
		exit(EXIT_FAILURE);
	} else if (c[0] != 'x' || (res == 2 && c[1] != ',' && c[1] != '\n')) {
		fputs("Bad puzzle input format\n", stderr);
		exit(EXIT_FAILURE);
	}
	(*rem)++;
}

static const Node *
parseids(FILE * const restrict in,
         uintmax_t * restrict rem,
         const uintmax_t mindep,
         uintmax_t * restrict bestdep,
         uintmax_t * restrict bestbus)
{
	Node *tail = NULL;
	while (!feof(in) && !ferror(in)) {
		uintmax_t bus;
		if (fscanf(in, "%ju", &bus) < 1) {
			if (feof(in) || ferror(in))
				break;
			retryinput(in, rem);
			continue;
		}
		if (bus == 0) {
			fputs("Bus has id 0\n", stderr);
			exit(EXIT_FAILURE);
		}
		uintmax_t departs = bus * (mindep / bus + (mindep % bus != 0));
		if (*bestdep == 0 || departs < *bestdep) {
			*bestbus = bus;
			*bestdep = departs;
		}
		Node * const node = malloc(sizeof(Node));
		if (node == NULL) {
			fputs("Could not allocate new node\n", stderr);
			exit(EXIT_FAILURE);
		}
		node->rem = (bus - ((*rem)++ % bus)) % bus;
		node->div = bus;
		node->next = NULL;
		if (head == NULL)
			head = node;
		else
			tail->next = node;
		tail = node;
		const int next = fgetc(in);
		if (next != ',' && next != '\n' && next != EOF) {
			fprintf(stderr, "Unexpected character: %c\n", next);
			exit(EXIT_FAILURE);
		}
	}
	if (!feof(in) || ferror(in)) {
		fputs("Error occured while parsing puzzle input\n", stderr);
		exit(EXIT_FAILURE);
	}
	return tail;
}

static void
freelist(void)
{
	while (head != NULL) {
		Node * const temp = head->next;
		free(head);
		head = temp;
	}
}

int
day13(FILE * const in)
{
	if (atexit(freelist) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	uintmax_t mindep;
	if (fscanf(in, "%ju", &mindep) != 1 || fgetc(in) != '\n') {
		fputs("Could not parse earliest timestamp\n", stderr);
		return EXIT_FAILURE;
	}
	uintmax_t bestbus, bestdep = 0, rem = 0;
	const Node * const tail = parseids(in, &rem, mindep, &bestdep, &bestbus);
	if (bestdep == 0) {
		fputs("No bus found\n", stderr);
		return EXIT_FAILURE;
	}
	printf("Product\t%ju\n", bestbus * (bestdep - mindep));
	while (head != tail) {
		Node * const a = head, * const b = head->next;
		intmax_t m[2];
		if (xgcd(a->div, b->div, m, m + 1) != 1)
			fputs("Divisors are not coprime\n", stderr);
		const uintmax_t n = a->div * b->div;
		b->rem = addmod(mulmod(a->rem, posmod(m[1], n), b->div, n),
		                mulmod(b->rem, posmod(m[0], n), a->div, n),
		                n);
		b->div *= a->div;
		free(a);
		head = b;
	}
	printf("Chinese\t%ju\n", head->rem);
	return EXIT_SUCCESS;
}
