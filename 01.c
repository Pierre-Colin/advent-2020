/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct Node {
	uint16_t value;
	struct Node *next;
};

typedef struct Node Node;

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

int
day01(FILE * const in)
{
	if (atexit(freelist) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	Node *tail = NULL;
	int inputerr;
	uint16_t input;
	while ((inputerr = fscanf(in, "%" SCNu16 "\n", &input)) == 1) {
		Node *node = malloc(sizeof(Node));
		if (node == NULL) {
			perror("Failed to allocate node");
			return EXIT_FAILURE;
		}
		node->value = input;
		node->next = NULL;
		if (head != NULL)
			tail->next = node;
		else
			head = node;
		tail = node;
	}
	if (inputerr != EOF) {
		fputs("Bad input format\n", stderr);
		return EXIT_FAILURE;
	}
	if (head == NULL) {
		fputs("Linked list is empty\n", stderr);
		return EXIT_FAILURE;
	}
	for (const Node *i = head; i->next != NULL; i = i->next) {
		for (const Node *j = i->next; j != NULL; j = j->next) {
			if (i->value + j->value == 2020) {
				printf("2\t%" PRIu32 "\n",
				       ((uint32_t) i->value)
				       * ((uint32_t) j->value));
			}
			if (j->next == NULL)
				continue;
			for (const Node *k = j->next; k != NULL; k = k->next) {
				if (i->value + j->value + k->value != 2020)
					continue;
				printf("3\t%" PRIu64 "\n",
				       ((uint64_t) i->value)
				       * ((uint64_t) j->value)
				       * ((uint64_t) k->value));
			}
		}
	}
	return EXIT_SUCCESS;
}
