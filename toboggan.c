/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PATTERN_WIDTH 31

struct Node {
	uint32_t line;
	struct Node *next;
};

typedef struct Node Node;

typedef struct {
	uint8_t right;
	uint8_t down;
} Slope;

static Node *head = NULL;

static void
freelist(void)
{
	Node *node = head;
	while (node != NULL) {
		Node *next = node->next;
		free(node);
		node = next;
	}
}

static Node *
advance(Node *node, uint8_t steps)
{
	while (node != NULL && steps-- > 0)
		node = node->next;
	return node;
}

int
day03(void)
{
	Node *tail = NULL;
	char input[PATTERN_WIDTH + 2];
	if (atexit(freelist) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	while (fgets(input, PATTERN_WIDTH + 2, stdin) != NULL) {
		uint32_t line = 0;
		for (uint8_t i = 0; i < PATTERN_WIDTH; i++) {
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
		Node *node = malloc(sizeof(Node));
		if (node == NULL) {
			perror("Could not allocate new node");
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
	Slope slopes[] = {
		{.right = 1, .down = 1},
		{.right = 3, .down = 1},
		{.right = 5, .down = 1},
		{.right = 7, .down = 1},
		{.right = 1, .down = 2}
	};
	uint64_t product = 1;
	for (uint8_t s = 0; s < sizeof(slopes) / sizeof(Slope); s++) {
		uint64_t trees = 0;
		uint64_t x = 0;
		Node *node;
		for (node = head; node; node = advance(node, slopes[s].down)) {
			if ((node->line >> (x % PATTERN_WIDTH)) & 1)
				trees++;
			x += slopes[s].right;
		}
		if (s == 1)
			printf("R3D1\t%" PRIu64 "\n", trees);
		product *= trees;
	}
	printf("Product\t%" PRIu64 "\n", product);
	return EXIT_SUCCESS;
}
