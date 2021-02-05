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
#include <string.h>

struct Node {
	uint64_t rem;
	uint64_t div;
	struct Node *next;
};

typedef struct Node Node;

static Node *head = NULL;

static uint64_t
xgcd(const int64_t a, const int64_t b, int64_t *x, int64_t *y)
{
	int64_t m[2][2];
	memset(m, 0, sizeof(m));
	m[0][0] = 1;
	m[1][1] = 1;
	uint64_t r[2] = { a, b };
	while (r[1] > 0) {
		const int64_t q = r[0] / r[1];
		const uint64_t rr = r[0] - q * r[1];
		const int64_t aa = m[0][0] - q * m[1][0];
		const int64_t bb = m[0][1] - q * m[1][1];
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

static uint64_t
posmod(int64_t x, const uint64_t n)
{
	if (x < 0)
		x += n * (-x / n + 1);
	return x % n;
}

static uint64_t
mul2mod(uint64_t x, uint64_t y, const uint64_t n)
{
	x %= n;
	y %= n;
	uint64_t r = 0;
	while (y > 0) {
		if (y % 2 == 1)
			r = (r + x) % n;
		x = (2 * x) % n;
		y /= 2;
	}
	return r;
}

static uint64_t
mulmod(uint64_t x, uint64_t y, uint64_t z, const uint64_t n)
{
	return mul2mod(mul2mod(x, y, n), z, n);
}

static uint64_t
addmod(const uint64_t x, const uint64_t y, const uint64_t n)
{
	return ((x % n) + (y % n)) % n;
}

static void
printinerr(void)
{
	if (errno != 0)
		perror("Input failed");
	else
		fputs("Bad input format\n", stderr);
}

static void
retryinput(FILE * const in, uint64_t *const rem)
{
	if (errno != 0) {
		perror("Input failed");
		exit(EXIT_FAILURE);
	}
	char c[2];
	if (fscanf(in, "%1[x]%*1[,\n]", c) != 1) {
		printinerr();
		exit(EXIT_FAILURE);
	}
	(*rem)++;
}

static void
freelist(void)
{
	Node *node = head;
	while (node != NULL) {
		Node *temp = node->next;
		free(node);
		node = temp;
	}
}

int
day13(FILE * const in)
{
	if (atexit(freelist) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	uint64_t mindep;
	int scanres;
	if ((scanres = fscanf(in, "%" SCNu64 "%*1[\n]", &mindep)) != 1) {
		printinerr();
		return EXIT_FAILURE;
	}
	uint64_t bestbus, bestdep = 0;
	uint64_t rem = 0;
	Node *tail = NULL;
	uint64_t bus;
	while ((scanres = fscanf(in, "%" SCNu64 "%*1[,\n]", &bus)) != EOF) {
		if (scanres < 1) {
			retryinput(in, &rem);
			continue;
		}
		if (bus == 0) {
			fputs("Bus has id 0\n", stderr);
			return EXIT_FAILURE;
		}
		uint64_t departs = bus * (mindep / bus + (mindep % bus != 0));
		if (bestdep == 0 || departs < bestdep) {
			bestbus = bus;
			bestdep = departs;
		}
		Node *node = malloc(sizeof(Node));
		if (node == NULL) {
			perror("Allocation failed");
			return EXIT_FAILURE;
		}
		node->rem = (bus - (rem++ % bus)) % bus;
		node->div = bus;
		node->next = NULL;
		if (head == NULL)
			head = node;
		else
			tail->next = node;
		tail = node;
	}
	if (bestdep == 0) {
		fputs("No bus found\n", stderr);
		return EXIT_FAILURE;
	}
	printf("Product\t%" PRIu64 "\n", bestbus * (bestdep - mindep));
	while (head != tail) {
		Node *a = head;
		Node *b = a->next;
		int64_t m[2];
		if (xgcd(a->div, b->div, m, m + 1) != 1)
			fputs("Divisors are not coprime\n", stderr);
		const uint64_t n = a->div * b->div;
		const uint64_t newrem = addmod(mulmod(a->rem, posmod(m[1], n), b->div, n),
		                               mulmod(b->rem, posmod(m[0], n), a->div, n),
		                               n);
		b->rem = newrem;
		b->div *= a->div;
		free(a);
		head = b;
	}
	printf("Chinese\t%" PRIu64 "\n", head->rem);
	return EXIT_SUCCESS;
}
