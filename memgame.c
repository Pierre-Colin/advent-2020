/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct Node {
	uint32_t val;
	uint32_t lastturn;
	uint32_t slastturn;
	struct Node *left;
	struct Node *right;
};

typedef struct Node Node;

static Node *root = NULL;

static Node *
insertleaf(Node **const node, const uint32_t val, const uint32_t turn)
{
	Node *new = malloc(sizeof(Node));
	if (new == NULL) {
		perror("Allocation failed");
		exit(EXIT_FAILURE);
	}
	new->val = val;
	new->lastturn = turn;
	new->slastturn = UINT32_MAX;
	new->left = NULL;
	new->right = NULL;
	*node = new;
	return new;
}

static Node *
insert(const uint32_t val, const uint32_t turn)
{
	if (root == NULL)
		return insertleaf(&root, val, turn);
	Node *node = root;
	for (;;) {
		if (val == node->val) {
			node->slastturn = node->lastturn;
			node->lastturn = turn;
			return node;
		} else if (val < node->val) {
			if (node->left == NULL)
				return insertleaf(&node->left, val, turn);
			else
				node = node->left;
		} else {
			if (node->right == NULL)
				return insertleaf(&node->right, val, turn);
			else
				node = node->right;
		}
	}
}

static Node *
playturn(Node *last, const uint32_t turn)
{
	if (last->slastturn == UINT32_MAX)
		return insert(UINT32_C(0), turn);
	return insert(last->lastturn - last->slastturn, turn);
}

static void
freenode(Node *node)
{
	if (node == NULL)
		return;
	freenode(node->left);
	freenode(node->right);
	free(node);
}

static void
freetree(void)
{
	freenode(root);
}

int
day15(void)
{
	if (atexit(freetree) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	uint32_t input;
	uint32_t turn = 0;
	Node *last = NULL;
	int scanres;
	while ((scanres = scanf("%" SCNu32, &input)) == 1) {
		last = insert(input, turn);
		switch (getchar()) {
		default:
			fputs("Bad input format\n", stderr);
			return EXIT_FAILURE;
		case ',':
		case '\n':
		case EOF:
			turn++;
		}
	}
	if (scanres != EOF) {
		if (errno != 0)
			perror("Input failed");
		else
			fputs("Bad input format\n", stderr);
		return EXIT_FAILURE;
	}
	if (last == NULL) {
		fputs("There were no starting numbers\n", stderr);
		return EXIT_FAILURE;
	}
	while (turn < 2020)
		last = playturn(last, turn++);
	printf("2020th\t%" PRIu32 "\n", last->val);
	while (turn < UINT32_C(30000000))
		last = playturn(last, turn++);
	printf("30Mth\t%" PRIu32 "\n", last->val);
	return EXIT_SUCCESS;
}

