/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* Linked list took ~15 s; binary tree takes ~50 ms */
struct Node {
	uint_fast64_t addr, val;
	struct Node *left, *right;
};

typedef struct Node Node;

static Node *head;

static void
freetree(Node *node)
{
	if (node == NULL)
		return;
	freetree(node->left);
	freetree(node->right);
	free(node);
}

/* Consumes the tree; this saves a couple ms */
static uintmax_t
sumtree(Node * const node)
{
	if (node == NULL)
		return 0;
	uintmax_t sum = node->val + sumtree(node->left) + sumtree(node->right);
	free(node->left);
	free(node->right);
	node->left = node->right = NULL;
	return sum;
}

static void
addnode(const uint_fast64_t addr, const uint_fast64_t val)
{
	Node * const node = malloc(sizeof(Node));
	if (node == NULL) {
		fputs("Could not allocate new node\n", stderr);
		exit(EXIT_FAILURE);
	}
	node->addr = addr;
	node->val = val;
	node->left = NULL;
	node->right = NULL;
	if (head == NULL) {
		head = node;
		return;
	}
	Node *it = head;
	for (;;) {
		if (addr == it->addr) {
			it->val = val;
			free(node);
			return;
		} else if (addr < it->addr) {
			if (it->left == NULL) {
				it->left = node;
				return;
			} else {
				it = it->left;
			}
		} else {
			if (it->right == NULL) {
				it->right = node;
				return;
			} else {
				it = it->right;
			}
		}
	}
}

static void
floataddr(uint_fast64_t addr,
          const bool *floating,
          uint_fast8_t bit,
          uint_fast64_t val)
{
	if (bit >= 36) {
		addnode(addr, val);
	} else if (!floating[bit]) {
		floataddr(addr, floating, bit + 1, val);
	} else {
		addr &= ~(UINT64_C(1) << bit);
		floataddr(addr, floating, bit + 1, val);
		addr |= UINT64_C(1) << bit;
		floataddr(addr, floating, bit + 1, val);
	}
}

static void
runmeminstr(const char input[restrict 48],
            uint_fast64_t mem[restrict 65536],
            uint_fast64_t mask)
{
	uint_fast64_t addr, val;
	if (sscanf(input, "mem[%" SCNuFAST64 "] = %" SCNuFAST64, &addr, &val)
	    != 2) {
		fprintf(stderr, "Bad input format: %s\n", input);
		exit(EXIT_FAILURE);
	}
	uint_fast64_t modaddr = addr, modval = val;
	bool floating[36] = { false };
	for (uint_fast8_t i = 0; i < 36; i++) {
		if (mask % 3 == 2) {
			modval |= UINT64_C(1) << i;
			modaddr |= UINT64_C(1) << i;
		} else if (mask % 3 == 1) {
			modval &= ~(UINT64_C(1) << i);
		} else {
			floating[i] = true;
		}
		mask /= 3;
	}
	floataddr(modaddr, floating, 0, val);
	mem[addr] = modval;
}

static bool
runmaskinstr(const char input[restrict 48],
             uint_fast64_t * const restrict mask)
{
	int n;
	sscanf(input, "mask = %*36[01X]%n", &n);
	if (n != 43)
		return false;
	uint_fast64_t nmask = 0;
	for (uint_fast8_t i = 0; i < 36; i++) {
		if (input[7 + i] == 0)
			return false;
		nmask *= 3;
		if ((input[7 + i] >> 1) == ('0' >> 1))
			nmask += input[7 + i] - '0' + 1;
	}
	if (input[43] == 0) {
		*mask = nmask;
		return true;
	}
	return false;
}

static void
freedata(void)
{
	freetree(head);
}

int
day14(FILE * const in)
{
	if (atexit(freedata) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	uint_fast64_t mask = 0, mem[65536] = { 0 };
	char input[48];
	while (fscanf(in, "%47[^\n]", input) == 1) {
		if (!runmaskinstr(input, &mask))
			runmeminstr(input, mem, mask);
		const int next = fgetc(in);
		if (next != '\n' && next != EOF) {
			fprintf(stderr, "Unexpected character: %c\n", next);
			return EXIT_FAILURE;
		}
	}
	if (!feof(in) || ferror(in)) {
		fputs("Error occured while parsing puzzle input\n", stderr);
		return EXIT_FAILURE;
	}
	uintmax_t sum = 0;
	for (uint_fast32_t i = 0; i < 65536; i++) {
		if (mem[i] > UINTMAX_MAX - sum)
			fputs("Integer wraparound detected\n", stderr);
		sum += mem[i];
	}
	printf("Ver 1\t%ju\n", sum);
	printf("Ver 2\t%ju\n", sumtree(head));
	return EXIT_SUCCESS;
}
