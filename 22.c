/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Upper bound to how many digits a type may hold */
#define DIGITS(T) (CHAR_BIT * 10 * sizeof(T) / (9 * sizeof(char)))

struct Card {
	uintmax_t val;
	struct Card *next;
};

typedef struct Card Card;

typedef struct {
	uintmax_t *a;
	size_t sz;
} CardSlice;

struct History {
	CardSlice deck[2];
	struct History *left, *right;
};

typedef struct History History;

static int recursivecombat_rec(Card *[2], Card *[2], uintmax_t[2], uintmax_t *);

static Card *card[2] = { NULL, NULL };
static size_t totalcards = 0;

static void
freelist(Card *list)
{
	while (list != NULL) {
		Card * const next = list->next;
		free(list);
		list = next;
	}
}

static void
freedecks(Card *deck[const 2])
{
	freelist(deck[0]);
	freelist(deck[1]);
}

static void
freehistory(History * const his)
{
	if (his == NULL)
		return;
	freehistory(his->left);
	freehistory(his->right);
	free(his->deck[0].a);
	free(his->deck[1].a);
	free(his);
}

static uintmax_t
consumespaces(FILE * const in, uintmax_t line)
{
	int c;
	while (isspace(c = fgetc(in))) {
		if (c == '\n')
			line++;
	}
	ungetc(c, in);
	return line;
}

static uintmax_t
parseplayer(FILE * const in, const uint_fast8_t pnum, uintmax_t line)
{
	int num;
	char buf[12];
	sprintf(buf, "Player %" PRIuFAST8 ":%%n", pnum + 1);
	fscanf(in, buf, &num);
	if (num == 0) {
		fprintf(stderr, "Invalid player header on line %ju\n", line);
		exit(EXIT_FAILURE);
	}
	Card *tail = NULL;
	for (;;) {
		uintmax_t val;
		line = consumespaces(in, line);
		if (fscanf(in, "%ju", &val) < 1)
			break;
		Card * const new = malloc(sizeof(Card));
		if (new == NULL) {
			fprintf(stderr, "%ju: Could not allocate card\n", line);
			exit(EXIT_FAILURE);
		}
		new->val = val;
		new->next = NULL;
		if (card[pnum] == NULL)
			card[pnum] = new;
		else
			tail->next = new;
		tail = new;
		totalcards++;
	}
	if (ferror(in)) {
		fprintf(stderr, "%ju: Internal error while parsing\n", line);
		exit(EXIT_FAILURE);
	}
	return line;
}

static bool
parse(FILE * const in)
{
	parseplayer(in, 1, parseplayer(in, 0, 1));
	return feof(in);
}

static uintmax_t
regularcombat(void)
{
	Card *head[2] = { NULL, NULL }, *tail[2] = { NULL, NULL };
	uintmax_t ncard = 0;
	for (size_t p = 0; p < 2; p++) {
		for (const Card *it = card[p]; it != NULL; it = it->next) {
			Card * const new = malloc(sizeof(Card));
			if (new == NULL) {
				fputs("Could not allocate card copy\n", stderr);
				freedecks(head);
				exit(EXIT_FAILURE);
			}
			new->val = it->val;
			new->next = NULL;
			if (head[p] == NULL)
				head[p] = new;
			else
				tail[p]->next = new;
			tail[p] = new;
			ncard++;
		}
	}
	while (head[0] != NULL && head[1] != NULL) {
		const size_t win = head[1]->val > head[0]->val, lose = !win;
		tail[win] = tail[win]->next = head[win];
		Card *temp = head[win]->next;
		head[win]->next = NULL;
		head[win] = temp;
		tail[win] = tail[win]->next = head[lose];
		temp = head[lose]->next;
		head[lose]->next = NULL;
		head[lose] = temp;
	}
	uintmax_t score = 0;
	const size_t win = head[1] != NULL;
	while (head[win] != NULL) {
		score += head[win]->val * ncard--;
		Card * const next = head[win]->next;
		free(head[win]);
		head[win] = next;
	}
	return score;
}

static bool
clonedeck(CardSlice * const slice, const Card *card)
{
	if ((slice->a = malloc(totalcards * sizeof(uintmax_t))) == NULL)
		return false;
	slice->sz = 0;
	while (card != NULL) {
		slice->a[slice->sz++] = card->val;
		card = card->next;
	}
	return true;
}

static int
cmpdeck(const Card *a, const CardSlice *b)
{
	size_t i = 0;
	while (a != NULL && i < b->sz && a->val == b->a[i]) {
		a = a->next;
		i++;
	}
	if (a == NULL && i >= b->sz)
		return 0;
	if (a == NULL)
		return -1;
	if (i >= b->sz)
		return 1;
	if (a->val < b->a[i])
		return -1;
	return 1;
}

static int
playround(Card *card[2], const uintmax_t ncard[2])
{
	if (ncard[0] <= card[0]->val || ncard[1] <= card[1]->val)
		return card[1]->val > card[0]->val;
	/* Recursive game */
	Card *copy[2] = { NULL, NULL }, *tail[2] = { NULL, NULL };
	uintmax_t ncard_copy[2] = { card[0]->val, card[1]->val };
	for (size_t p = 0; p < 2; p++) {
		const Card *it = card[p]->next;
		for (uintmax_t i = 0; i < ncard_copy[p]; i++) {
			Card * const new = malloc(sizeof(Card));
			if (new == NULL) {
				freedecks(copy);
				return -1;
			}
			new->val = it->val;
			new->next = NULL;
			if (copy[p] == NULL)
				copy[p] = new;
			else
				tail[p]->next = new;
			tail[p] = new;
			it = it->next;
		}
	}
	const int result = recursivecombat_rec(copy, tail, ncard_copy, NULL);
	freedecks(copy);
	return result;
}

static int
cmphistory(Card *lhs[2], const CardSlice rhs[2])
{
	const int cmp = cmpdeck(lhs[0], rhs);
	return cmp != 0? cmp : cmpdeck(lhs[1], rhs + 1);
}

static bool
addleaf(History ** const ptr, Card *deck[2])
{
	History * const new = malloc(sizeof(History));
	if (new == NULL)
		return false;
	if (!clonedeck(new->deck, deck[0])) {
		free(new);
		return false;
	}
	if (!clonedeck(new->deck + 1, deck[1])) {
		free(new->deck[0].a);
		free(new);
		return false;
	}
	new->left = new->right = NULL;
	*ptr = new;
	return true;
}

static int
checkhistory(History ** const his, Card *deck[2])
{
	if (*his == NULL)
		return addleaf(his, deck)? 0 : -1;
	History *it = *his;
	for (;;) {
		const int cmp = cmphistory(deck, it->deck);
		if (cmp == 0)
			return 1;
		if (cmp < 0) {
			if (it->left == NULL)
				return addleaf(&it->left, deck)? 0 : -1;
			it = it->left;
		} else {
			if (it->right == NULL)
				return addleaf(&it->right, deck)? 0 : -1;
			it = it->right;
		}
	}
}

static int
recursivecombat_rec(Card *head[2],
                    Card *tail[2],
                    uintmax_t ncard[restrict 2],
                    uintmax_t * const restrict score)
{
	History *his = NULL;
	while (head[0] != NULL && head[1] != NULL) {
		const int hischeck = checkhistory(&his, head);
		if (hischeck < 0) {
			freehistory(his);
			return -1;
		} else if (hischeck > 0) {
			/* Player 1 wins */
			freehistory(his);
			if (score != NULL) {
				while (head[0] != NULL) {
					*score += head[0]->val * ncard[0]--;
					head[0] = head[0]->next;
				}
			}
			return 0;
		}
		const int win = playround(head, ncard), lose = !win;
		if (win < 0) {
			freehistory(his);
			return -1;
		}
		tail[win] = tail[win]->next = head[win];
		Card *temp = head[win]->next;
		head[win]->next = NULL;
		head[win] = temp;
		tail[win] = tail[win]->next = head[lose];
		temp = head[lose]->next;
		head[lose]->next = NULL;
		head[lose] = temp;
		ncard[win]++;
		ncard[lose]--;
		if (head[lose] == NULL)
			break;
	}
	freehistory(his);
	int win = head[1] != NULL;
	if (score != NULL) {
		for (const Card *c = head[win]; c != NULL; c = c->next)
			*score += c->val * ncard[win]--;
	}
	return win;
}

static int
recursivecombat(uintmax_t * const score)
{
	Card *tail[2];
	uintmax_t ncard[2] = { 1, 1 };
	for (size_t p = 0; p < 2; p++) {
		for (tail[p] = card[p]; tail[p]->next; tail[p] = tail[p]->next)
			ncard[p]++;
	}
	return recursivecombat_rec(card, tail, ncard, score);
}

static void
freedata(void)
{
	freedecks(card);
}

int
day22(FILE * const in)
{
	if (atexit(freedata) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	if (!parse(in)) {
		fputs("Did not parse the entire puzzle input\n", stderr);
		return EXIT_FAILURE;
	}
	if (card[0] == NULL || card[1] == NULL) {
		fputs("At least one deck is empty\n", stderr);
		return EXIT_FAILURE;
	}
	printf("Regular\t%ju\n", regularcombat());
	uintmax_t score = 0;
	if (recursivecombat(&score) < 0) {
		fputs("Could not unroll recursive combat game\n", stderr);
		return EXIT_FAILURE;
	}
	printf("Recurs\t%ju\n", score);
	return EXIT_SUCCESS;
}
