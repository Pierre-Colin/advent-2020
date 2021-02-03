/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Upper bound to how many digits a type may hold */
#define DIGITS(T) (CHAR_BIT * 10 * sizeof(T) / (9 * sizeof(char)))

#define PRINT_ERR(s) \
	if (errno != 0) \
		perror(s); \
	else \
		fputs(s "\n", stderr);

struct Card {
	uintmax_t val;
	struct Card *next;
};

typedef struct Card Card;

struct History {
	Card *deck[2];
	struct History *next;
};

typedef struct History History;

static int recursivecombat_rec(Card *[2], Card *[2], uintmax_t[2], uintmax_t *);

static Card *card[2] = { NULL, NULL };

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
freedecks(Card *deck[2])
{
	freelist(deck[0]);
	freelist(deck[1]);
}

static void
freehistory(History *his)
{
	while (his != NULL) {
		freedecks(his->deck);
		History * const next = his->next;
		free(his);
		his = next;
	}
}

static void
parsingerror(const char * const str, const uintmax_t line)
{
	if (errno != 0) {
		char buf[strlen(str) + 10 + DIGITS(uintmax_t)];
		sprintf(buf, "%s on line %" PRIuMAX, str, line);
		perror(buf);
	} else {
		fprintf(stderr, "%s on line %" PRIuMAX "\n", str, line);
	}
}

static uintmax_t
consumespaces(uintmax_t line)
{
	int c;
	while (isspace(c = getchar())) {
		if (c == '\n')
			line++;
	}
	ungetc(c, stdin);
	return line;
}

static uintmax_t
parseplayer(const uint_fast8_t pnum, uintmax_t line)
{
	int num;
	char buf[12];
	sprintf(buf, "Player %" PRIuFAST8 ":%%n", pnum + 1);
	scanf(buf, &num);
	if (num == 0) {
		fprintf(stderr, "Invalid player header on line %ju\n", line);
		exit(EXIT_FAILURE);
	}
	Card *tail = NULL;
	for (;;) {
		uintmax_t val;
		line = consumespaces(line);
		if (scanf("%ju", &val) < 1)
			break;
		Card * const new = malloc(sizeof(Card));
		if (new == NULL) {
			parsingerror("Could not allocate a new card", line);
			exit(EXIT_FAILURE);
		}
		new->val = val;
		new->next = NULL;
		if (card[pnum] == NULL)
			card[pnum] = new;
		else
			tail->next = new;
		tail = new;
	}
	if (ferror(stdin)) {
		parsingerror("Puzzle input failed", line);
		exit(EXIT_FAILURE);
	}
	return line;
}

static bool
parse(void)
{
	parseplayer(1, parseplayer(0, 1));
	return feof(stdin);
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
				PRINT_ERR("Could not copy cards");
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

static Card *
clonedeck(const Card *card)
{
	Card *head = NULL, *tail = NULL;
	while (card != NULL) {
		Card * const new = malloc(sizeof(Card));
		if (new == NULL) {
			freelist(head);
			return NULL;
		}
		new->val = card->val;
		new->next = NULL;
		if (head == NULL)
			head = new;
		else
			tail->next = new;
		tail = new;
		card = card->next;
	}
	return head;
}

static bool
deckeq(const Card *a, const Card *b)
{
	while (a != NULL && b != NULL) {
		if (a == NULL || b == NULL || a->val != b->val)
			return false;
		a = a->next;
		b = b->next;
	}
	return true;
}

static bool
alreadyvisited(Card *deck[2], const History *his)
{
	while (his != NULL) {
		if (deckeq(deck[0], his->deck[0])
		    && deckeq(deck[1], his->deck[1]))
			return true;
		his = his->next;
	}
	return false;
}

static int
playround(Card *card[2], const uintmax_t ncard[2])
{
	if (ncard[0] > card[0]->val && ncard[1] > card[1]->val) {
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
		int result = recursivecombat_rec(copy, tail, ncard_copy, NULL);
		freedecks(copy);
		return result;
	}
	return card[1]->val > card[0]->val;
}

static int
recursivecombat_rec(Card *head[2],
                    Card *tail[2],
                    uintmax_t ncard[2],
                    uintmax_t * const score)
{
	History *his = malloc(sizeof(History));
	if (his == NULL || (his->deck[0] = clonedeck(head[0])) == NULL) {
		free(his);
		return -1;
	}
	if ((his->deck[1] = clonedeck(head[1])) == NULL) {
		freelist(his->deck[0]);
		free(his);
		return -1;
	}
	his->next = NULL;
	while (head[0] != NULL && head[1] != NULL) {
		if (alreadyvisited(head, his->next)) {
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
		History * const new = malloc(sizeof(History));
		if (new == NULL) {
			freehistory(his);
			return -1;
		}
		if ((new->deck[0] = clonedeck(head[0])) == NULL) {
			free(new);
			freehistory(his);
			return -1;
		}
		if ((new->deck[1] = clonedeck(head[1])) == NULL) {
			freelist(new->deck[0]);
			free(new);
			freehistory(his);
			return -1;
		}
		new->next = his;
		his = new;
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
day22(void)
{
	if (atexit(freedata) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	errno = 0;
	if (!parse()) {
		fputs("Did not parse the entire puzzle input\n", stderr);
		return EXIT_FAILURE;
	}
	if (card[0] == NULL || card[1] == NULL) {
		fputs("Decks are not non empty\n", stderr);
		return EXIT_FAILURE;
	}
	printf("Regular\t%ju\n", regularcombat());
	uintmax_t score;
	if (recursivecombat(&score) < 0) {
		PRINT_ERR("Failed to unroll recursive combat game");
		return EXIT_FAILURE;
	}
	printf("Recurs\t%ju\n", score);
	return EXIT_SUCCESS;
}

