/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum { TEOF, TEOL, PCLOSE, MULT, PLUS, POPEN, NUMBER } TokenType;

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
push(Node ** const stack, const uintmax_t val)
{
	Node * const new = malloc(sizeof(Node));
	if (new == NULL)
		return false;
	new->val = val;
	new->next = *stack;
	*stack = new;
	return true;
}

static bool
pop(Node ** const restrict stack, uintmax_t * const restrict result)
{
	if (*stack == NULL)
		return false;
	Node * const top = *stack;
	*stack = top->next;
	*result = top->val;
	free(top);
	return true;
}

static void
freestack(Node *stack)
{
	while (stack != NULL) {
		Node * const temp = stack->next;
		free(stack);
		stack = temp;
	}
}

static int
opcomp(const TokenType lhs, const TokenType rhs)
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
		fprintf(stderr, "%ju + %ju causes integer wraparound\n", x, y);
		return true;
	}
	return false;
}

static bool
emitop(Node ** const restrict acc,
       Node ** const restrict ops,
       const TokenType op)
{
	if (*ops == NULL || opcomp(op, (*ops)->val) >= 0)
		return push(ops, op);
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
				fputs("Integer wraparound detected\n", stderr);
				return false;
			}
			(*acc)->val *= val;
			break;
		default:
			fprintf(stderr, "%ju was in ops\n", topop);
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
nexttok(FILE * const restrict in, Token * const restrict tok)
{
	int c;
	while ((c = fgetc(in)) == ' ');
	ungetc(c, in);
	if (isdigit(c)) {
		tok->type = NUMBER;
		return fscanf(in, "%ju", &tok->val) == 1;
	}
	switch (c) {
	default:
		return false;
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
	}
	fgetc(in);
	return true;
}

static bool
isoperator(const TokenType t)
{
	return t == PLUS || t == MULT || t == TEOL || t == PCLOSE;
}

static bool
badtoken(const uintmax_t pos, const TokenType t)
{
	return (pos % 2) ^ isoperator(t);
}

static void
freeandexit(Node * const restrict acc, Node * const restrict ops)
{
	freestack(acc);
	freestack(ops);
	exit(EXIT_FAILURE);
}

static void
flatapplyop(uintmax_t * const res,
            const uintmax_t val,
            const TokenType type)
{
	if (type == PLUS)
		*res += val;
	if (type == MULT)
		*res *= val;
	if (type == TEOF)
		*res = val;
}

static bool
subexpr(FILE * const restrict in,
        const uintmax_t level,
        const uintmax_t line,
        uintmax_t * const restrict flatres,
        uintmax_t * const restrict stackres)
{
	Token t;
	uintmax_t pos = 0;
	TokenType lastop = TEOF;
	Node *acc = NULL, *ops = NULL;
	do {
		uintmax_t subvalflat = 0, subvalstack = 0;
		if (!nexttok(in, &t))
			freeandexit(acc, ops);
		if (badtoken(pos++, t.type)) {
			fprintf(stderr, "Syntax error on line %ju\n", line);
			freeandexit(acc, ops);
		}
		if (t.type == POPEN
		    && !subexpr(in, level + 1, line, &subvalflat, &subvalstack))
			freeandexit(acc, ops);
		if (t.type == NUMBER)
			subvalflat = subvalstack = t.val;
		flatapplyop(flatres, subvalflat, lastop);
		if (isoperator(t.type)) {
			lastop = t.type;
			if (!emitop(&acc, &ops, t.type)) {
				fputs("Could not emit operator\n", stderr);
				freeandexit(acc, ops);
			}
		} else {
			lastop = TEOL;
			if (!push(&acc, subvalstack)) {
				fputs("Could not emit number\n", stderr);
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
		fputs("More than one number remain in accumulator\n", stderr);
		freestack(acc);
		exit(EXIT_FAILURE);
	}
	*stackres = acc->val;
	freestack(acc);
	return t.type != TEOF;
}

static bool
parseexpr(FILE * const restrict in,
          uintmax_t * const restrict line,
          uintmax_t * const restrict flatacc,
          uintmax_t * const restrict stackacc)
{
	uintmax_t flatres = 0, stackres = 0;
	const bool cont = subexpr(in, 0, *line, &flatres, &stackres);
	if (!cont || addwilloverflow(*flatacc, flatres))
		return false;
	*flatacc += flatres;
	if (addwilloverflow(*stackacc, stackres))
		return false;
	*stackacc += stackres;
	(*line)++;
	return true;
}

int
day18(FILE * const in)
{
	uintmax_t line = 1, flatacc = 0, stackacc = 0;
	while (parseexpr(in, &line, &flatacc, &stackacc));
	if (!feof(in) || ferror(in)) {
		fprintf(stderr, "Puzzle input failed on line %ju\n", line);
		return EXIT_FAILURE;
	}
	printf("Flat\t%ju\n", flatacc);
	printf("Stack\t%ju\n", stackacc);
	return EXIT_SUCCESS;
}
