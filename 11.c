/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	HAS_SEAT = 1,
	SEATING = 2,
	WILL_SEAT = 4
};

struct Node {
	uint64_t *line;
	struct Node *next;
	struct Node *prev;
};

typedef struct Node Node;

static Node *head = NULL;

static void
freelist(void)
{
	Node *node = head;
	while (node != NULL) {
		free(node->line);
		Node *temp = node->next;
		free(node);
		node = temp;
	}
}

static bool
hasflag(const uint64_t *const line, const uint64_t flag, const size_t i)
{
	return (line[i / 21] & (flag << (3 * (i % 21)))) != 0;
}

static void
setflag(uint64_t *const line, const uint64_t flag, const size_t i)
{
	line[i / 21] |= flag << (3 * (i % 21));
}

static void
unsetflag(uint64_t *const line, const uint64_t flag, const size_t i)
{
	line[i / 21] &= ~(flag << (3 * (i % 21)));
}

static void
dupflag(uint64_t *const line, uint64_t dest, uint64_t src, const size_t i)
{
	uint64_t pattern = line[i / 21] & (src << (3 * (i % 21)));
	if (pattern == 0)
		line[i / 21] &= ~(dest << (3 * (i % 21)));
	else
		line[i / 21] |= dest << (3 * (i % 21));
}

static uint8_t
iteradjother(const Node *const other, const size_t i, const size_t width)
{
	if (other == NULL)
		return 0;
	uint8_t neighbors = 0;
	for (size_t j = (i > 0)? i - 1:0; j <= i + 1 && j < width; j++) {
		if (hasflag(other->line, SEATING, j))
			neighbors++;
	} 
	return neighbors;
}

static bool
treatadj(const Node *const node, const size_t width, const size_t i)
{
	if (!hasflag(node->line, HAS_SEAT, i))
		return false;
	uint8_t neighbors = iteradjother(node->prev, i, width)
	                    + iteradjother(node->next, i, width);
	if (i > 0 && hasflag(node->line, SEATING, i - 1))
		neighbors++;
	if (i < width - 1 && hasflag(node->line, SEATING, i + 1))
		neighbors++;
	if (!hasflag(node->line, SEATING, i) && neighbors == 0) {
		setflag(node->line, WILL_SEAT, i);
		return true;
	} else if (hasflag(node->line, SEATING, i) && neighbors >= 4) {
		unsetflag(node->line, WILL_SEAT, i);
		return true;
	}
	return false;
}

static bool
iteradjacent(const size_t width)
{
	bool changed = false;
	for (Node *node = head; node != NULL; node = node->next) {
		for (size_t i = 0; i < width; i++) {
			if (treatadj(node, width, i))
				changed = true;
		}
	}
	for (Node *node = head; node != NULL; node = node->next) {
		for (size_t i = 0; i < width; i++)
			dupflag(node->line, SEATING, WILL_SEAT, i);
	}
	return changed;
}

static bool
lookdirection(const Node *const node,
              const size_t width,
              const size_t i,
              const int8_t vdir,
              const int8_t hdir)
{
	const Node *other = node;
	size_t j = i;
	for (;;) {
		if (vdir > 0)
			other = other->next;
		else if (vdir < 0)
			other = other->prev;
		if (other == NULL)
			return false;
		if (j == 0 && hdir < 0)
			return false;
		else if (j + hdir >= width)
			return false;
		j += hdir;
		if (hasflag(other->line, HAS_SEAT, j)) {
			return hasflag(other->line, SEATING, j);
		}
	}
}

static uint8_t
lookaround(const Node *const node, const size_t width, const size_t i)
{
	uint8_t seen = 0;
	for (int8_t dx = -1; dx <= 1; dx++) {
		for (int8_t dy = -1; dy <= 1; dy++) {
			if (dx == 0 && dy == 0)
				continue;
			if (lookdirection(node, width, i, dy, dx))
				seen++;
		}
	}
	return seen;
}

static bool
treatseen(const Node *const node, const size_t width, const size_t i)
{
	if (!hasflag(node->line, HAS_SEAT, i))
		return false;;
	const uint8_t seen = lookaround(node, width, i);
	if (!hasflag(node->line, SEATING, i) && seen == 0) {
		setflag(node->line, WILL_SEAT, i);
		return true;
	} else if (hasflag(node->line, SEATING, i) && seen >= 5) {
		unsetflag(node->line, WILL_SEAT, i);
		return true;
	}
	return false;
}

static bool
iterseen(const size_t width)
{
	bool changed = false;
	for (Node *node = head; node != NULL; node = node->next) {
		for (size_t i = 0; i < width; i++) {
			if (treatseen(node, width, i))
				changed = true;
		}
	}
	for (Node *node = head; node != NULL; node = node->next) {
		for (size_t i = 0; i < width; i++)
			dupflag(node->line, SEATING, WILL_SEAT, i);
	}
	return changed;
}

static uint64_t
countoccupied(const size_t width)
{
	uint64_t occupied = 0;
	for (Node *node = head; node != NULL; node = node->next) {
		for (size_t i = 0; i < width; i++) {
			if (hasflag(node->line, SEATING, i))
				occupied++;
		}
	}
	return occupied;
}

int
day11(FILE * const in)
{
	if (atexit(freelist) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	Node *tail = NULL;
	size_t width = 0;
	char *input;
	int scanres;
	while ((scanres = fscanf(in, "%m[.L]%*1[\n]", &input)) != EOF) {
		if (scanres < 1) {
			if (errno != 0)
				perror("Input failed");
			else
				fputs("Bad input format\n", stderr);
			return EXIT_FAILURE;
		}
		if (width == 0) {
			width = strlen(input);
		} else if (strlen(input) != width) {
			fputs("Inconsistent input width\n", stderr);
			free(input);
			return EXIT_FAILURE;
		}
		uint64_t *line = malloc((width / 21 + 1) * sizeof(uint64_t));
		if (line == NULL) {
			perror("Allocation failed");
			free(input);
			return EXIT_FAILURE;
		}
		memset(line, 0, (width / 21 + 1) * sizeof(uint64_t));
		for (size_t i = 0; i < width; i++) {
			if (input[i] == 'L')
				line[i / 21] |= ((uint64_t) HAS_SEAT)
				                << (3 * (i % 21));
		}
		free(input);
		Node *new = malloc(sizeof(Node));
		if (new == NULL) {
			perror("Allocation failed");
			free(line);
			return EXIT_FAILURE;
		}
		new->line = line;
		new->next = NULL;
		if (head == NULL) {
			head = new;
			new->prev = NULL;
		} else {
			tail->next = new;
			new->prev = tail;
		}
		tail = new;
	}
	while (iteradjacent(width));
	printf("Adj\t%" PRIu64 "\n", countoccupied(width));
	for (Node *node = head; node != NULL; node = node->next) {
		for (size_t i = 0; i < width; i++)
			unsetflag(node->line, SEATING | WILL_SEAT, i);
	}
	while(iterseen(width));
	printf("Seen\t%" PRIu64 "\n", countoccupied(width));
	return EXIT_SUCCESS;
}
