/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PATTERN_WIDTH 31

struct Node {
	uint_fast32_t line;
	struct Node *next;
};

typedef struct Node Node;

typedef struct {
	uint_fast8_t right;
	uint_fast8_t down;
} Slope;

static Node *head = NULL;

static void
freelist(void)
{
	while (head != NULL) {
		Node * const next = head->next;
		free(head);
		head = next;
	}
}

static const Node *
advance(const Node *node, uint_fast8_t steps)
{
	while (node != NULL && steps-- > 0)
		node = node->next;
	return node;
}

int
day03(FILE * const in)
{
	Node *tail = NULL;
	char input[PATTERN_WIDTH + 2];
	if (atexit(freelist) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	errno = 0;
	while (fgets(input, PATTERN_WIDTH + 2, in) != NULL) {
		uint_fast32_t line = 0;
		for (uint_fast8_t i = 0; i < PATTERN_WIDTH; i++) {
			switch (input[i]) {
			case '#':
				line |= 1 << i;
			case '.':
				break;
			default:
				fputs("Bad input format\n", stderr);
				return EXIT_FAILURE;
			}
		}
		if (input[PATTERN_WIDTH] != '\n') {
			fputs("Bad input format\n", stderr);
			return EXIT_FAILURE;
		}
		Node * const node = malloc(sizeof(Node));
		if (node == NULL) {
			if (errno != 0)
				perror("Could not allocate new node");
			else
				fputs("Could not allocate new node\n", stderr);
			return EXIT_FAILURE;
		}
		node->line = line;
		node->next = NULL;
		if (head == NULL)
			head = node;
		else
			tail->next = node;
		tail = node;
	}
	if (!feof(in)) {
		if (errno != 0)
			perror("Could not parse puzzle input");
		else
			fputs("Could not parse puzzle input\n", stderr);
		return EXIT_FAILURE;
	}
	Slope slopes[] = {
		{ .right = 1, .down = 1 },
		{ .right = 3, .down = 1 },
		{ .right = 5, .down = 1 },
		{ .right = 7, .down = 1 },
		{ .right = 1, .down = 2 }
	};
	uintmax_t product = 1;
	for (uint_fast8_t s = 0; s < sizeof(slopes) / sizeof(Slope); s++) {
		uintmax_t trees = 0, x = 0;
		const Node *node;
		for (node = head; node; node = advance(node, slopes[s].down)) {
			if ((node->line >> (x % PATTERN_WIDTH)) & 1)
				trees++;
			x += slopes[s].right;
		}
		if (s == 1)
			printf("R3D1\t%ju\n", trees);
		product *= trees;
	}
	printf("Product\t%ju\n", product);
	return EXIT_SUCCESS;
}
