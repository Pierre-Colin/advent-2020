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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PRINT_ERR(s) \
	do { \
		if (errno != 0) \
			perror(s); \
		else \
			fputs(s "\n", stderr); \
	} while (false)

typedef enum {
	TEOF,
	TEOL,
	PCLOSE,
	MULT,
	PLUS,
	POPEN,
	NUMBER
} TokenType;

typedef struct {
	TokenType type;
	uintmax_t val;
} Token;

struct Node {
	uintmax_t val;
	struct Node *next;
};

typedef struct Node Node;

static bool
push(Node **stack, uintmax_t val)
{
	errno = 0;
	Node *new = malloc(sizeof(Node));
	if (new == NULL)
		return false;
	new->val = val;
	new->next = *stack;
	*stack = new;
	return true;
}

static bool
pop(Node **stack, uintmax_t *result)
{
	if (*stack == NULL)
		return false;
	Node *top = *stack;
	*stack = top->next;
	*result = top->val;
	free(top);
	return true;
}

static void
freestack(Node *stack)
{
	while (stack != NULL) {
		Node *temp = stack->next;
		free(stack);
		stack = temp;
	}
}

static int
opcomp(TokenType lhs, TokenType rhs)
{
	if (lhs > rhs)
		return 1;
	if (lhs < rhs)
		return -1;
	return 0;
}

static bool
addwilloverflow(const uintmax_t x, const uintmax_t y)
{
	if (x > UINTMAX_MAX - y) {
		fprintf(stderr,
		        "Integer overflow detected "
		        "(%" PRIuMAX " + %" PRIuMAX ")\n",
		        x,
		        y);
		errno = ERANGE;
		return true;
	}
	return false;
}

static bool
emitop(Node **acc, Node **ops, TokenType op)
{
	if (*ops == NULL || opcomp(op, (*ops)->val) >= 0) {
		if (!push(ops, op))
			return false;
		return true;
	}
	while (*acc != NULL && *ops != NULL && opcomp(op, (*ops)->val) < 0) {
		uintmax_t val, topop = 1;
		pop(acc, &val);
		pop(ops, &topop);
		if (*acc == NULL) {
			fputs("ops was too big\n", stderr);
			return false;
		}
		switch (topop) {
		case PLUS:
			if (addwilloverflow((*acc)->val, val))
				return false;
			(*acc)->val += val;
			break;
		case MULT:
			if ((*acc)->val >= UINTMAX_MAX / val) {
				fputs("Integer overflow detected\n", stderr);
				return false;
			}
			(*acc)->val *= val;
			break;
		default:
			fprintf(stderr, "%" PRIuMAX " was in ops\n", topop);
			return false;
		}
	}
	if (acc == NULL) {
		fputs("Accumulator ended up empty\n", stderr);
		return false;
	}
	return !(op != TEOF && op != TEOL && op != PCLOSE && !push(ops, op));
}

static bool
nexttok(Token *tok)
{
	int c;
	errno = 0;
	while ((c = getchar()) == ' ');
	ungetc(c, stdin);
	if (isdigit(c)) {
		tok->type = NUMBER;
		return scanf("%" SCNuMAX, &tok->val) == 1;
	}
	switch (c) {
	case EOF:
		tok->type = TEOF;
		break;
	case '\n':
		tok->type = TEOL;
		break;
	case '+':
		tok->type = PLUS;
		break;
	case '*':
		tok->type = MULT;
		break;
	case '(':
		tok->type = POPEN;
		break;
	case ')':
		tok->type = PCLOSE;
		break;
	default:
		errno = EDOM;
		return false;
	}
	getchar();
	return true;
}

static bool
isoperator(TokenType t)
{
	return t == PLUS || t == MULT || t == TEOL || t == PCLOSE;
}

static bool
badtoken(uintmax_t pos, TokenType t)
{
	return (pos % 2 == 1 && !isoperator(t))
	       || (pos % 2 == 0 && isoperator(t));
}

static void
freeandexit(Node *acc, Node *ops)
{
	freestack(acc);
	freestack(ops);
	exit(EXIT_FAILURE);
}

static void
flatapplyop(uintmax_t *res, uintmax_t val, TokenType type)
{
	if (type == PLUS)
		*res += val;
	if (type == MULT)
		*res *= val;
	if (type == TEOF)
		*res = val;
}

static bool
subexpr(uintmax_t level,
        uintmax_t line,
        uintmax_t *flatres,
        uintmax_t *stackres)
{
	Token t;
	uintmax_t pos = 0;
	TokenType lastop = TEOF;
	Node *acc = NULL, *ops = NULL;
	do {
		uintmax_t subvalflat = 0, subvalstack = 0;
		if (!nexttok(&t))
			freeandexit(acc, ops);
		if (badtoken(pos++, t.type)) {
			fprintf(stderr,
			        "Syntax error on line %" PRIuMAX "\n",
			        line);
			freeandexit(acc, ops);
		}
		if (t.type == POPEN
		    && !subexpr(level + 1, line, &subvalflat, &subvalstack))
			freeandexit(acc, ops);
		if (t.type == NUMBER)
			subvalflat = subvalstack = t.val;
		flatapplyop(flatres, subvalflat, lastop);
		if (isoperator(t.type)) {
			lastop = t.type;
			if (!emitop(&acc, &ops, t.type)) {
				PRINT_ERR("Failed to push operator to stack");
				freeandexit(acc, ops);
			}
		} else {
			lastop = TEOL;
			if (!push(&acc, subvalstack)) {
				PRINT_ERR("Failed to push number on stack");
				freeandexit(acc, ops);
			}
		}
	} while (t.type != TEOF && ((level == 0 && t.type != TEOL)
	                            || (level > 0 && t.type != PCLOSE)));
	if (ops != NULL) {
		fputs("Operator stack ended up not empty\n", stderr);
		freeandexit(acc, ops);
	}
	if (acc == NULL) {
		fputs("Accumulator ended up empty\n", stderr);
		exit(EXIT_FAILURE);
	} else if (acc->next != NULL) {
		fputs("More than 1 number remain in accumulator\n", stderr);
		freestack(acc);
		exit(EXIT_FAILURE);
	}
	*stackres = acc->val;
	freestack(acc);
	return t.type != TEOF;
}

static bool
parseexpr(uintmax_t *line, uintmax_t *flatacc, uintmax_t *stackacc)
{
	errno = 0;
	uintmax_t flatres = 0, stackres = 0;
	const bool cont = subexpr(0, *line, &flatres, &stackres);
	if (!cont || errno != 0)
		return false;
	if (addwilloverflow(*flatacc, flatres))
		return false;
	*flatacc += flatres;
	if (addwilloverflow(*stackacc, stackres))
		return false;
	*stackacc += stackres;
	(*line)++;
	return true;
}

int
day18(void)
{
	uintmax_t line = 1;
	uintmax_t flatacc = 0, stackacc = 0;
	while (parseexpr(&line, &flatacc, &stackacc));
	if (ferror(stdin) || errno != 0) {
		char buf[64];
		snprintf(buf, 64, "Bad input on line %" PRIuMAX, line);
		if (errno != 0)
			perror(buf);
		else
			fprintf(stderr, "%s\n", buf);
		return EXIT_FAILURE;
	}
	printf("Flat\t%" PRIuMAX "\n", flatacc);
	printf("Stack\t%" PRIuMAX "\n", stackacc);
	return EXIT_SUCCESS;
}

