/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Upper bound to how many digits a given type may hold */
#define DIGITS(T) (CHAR_BIT * 10 * sizeof(T) / (9 * sizeof(char)))

#define GRAMMAR_REGEX "[0-9]+( \\| [0-9]+| [0-9]+)*|\"([ab])\""

typedef enum { GRAMMAR, MESSAGES } ParseState;

struct Message {
	char *msg;
	struct Message *next;
};

typedef struct Message Message;

typedef enum { CHARACTER, SEQUENCE } RuleType;

struct Symbol {
	uintmax_t num;
	struct Symbol *next;
};

typedef struct Symbol Symbol;

struct Sequence {
	Symbol *val;
	struct Sequence *next;
};

typedef struct Sequence Sequence;

typedef struct {
	uintmax_t num;
	RuleType type;
	union { char ch; Sequence *seq; } val;
} Rule;

struct RuleTree {
	uintmax_t num;
	bool flag;
	RuleType type;
	union { char ch; Sequence *seq; } val;
	struct RuleTree *left, *right;
};

typedef struct RuleTree RuleTree;

static Message *msg = NULL;
static RuleTree *root = NULL;

static bool
addrule(RuleTree *rule)
{
	if (root == NULL) {
		root = rule;
		return false;
	}
	RuleTree *it = root;
	for (;;) {
		if (rule->num < it->num) {
			if (it->left == NULL) {
				it->left = rule;
				return false;
			}
			it = it->left;
		} else if (rule->num > it->num) {
			if (it->right == NULL) {
				it->right = rule;
				return false;
			}
			it = it->right;
		} else {
			return true;
		}
	}
}

static RuleTree *
getrule(const uintmax_t num)
{
	RuleTree *it = root;
	while (it != NULL && it->num != num) {
		if (num < it->num)
			it = it->left;
		else
			it = it->right;
	}
	return it;
}

static void
freesymbols(Symbol *sym)
{
	while (sym != NULL) {
		Symbol * const next = sym->next;
		free(sym);
		sym = next;
	}
}

static void
freeseq(Sequence *seq)
{
	while (seq != NULL) {
		Sequence * const next = seq->next;
		freesymbols(seq->val);
		free(seq);
		seq = next;
	}
}

static Symbol *
clonesymb(const Symbol * restrict sym)
{
	Symbol *head = NULL, *tail = NULL;
	while (sym != NULL) {
		Symbol * const new = malloc(sizeof(Symbol));
		if (new == NULL) {
			freesymbols(head);
			return NULL;
		}
		new->num = sym->num;
		new->next = NULL;
		if (head == NULL)
			head = new;
		else
			tail->next = new;
		tail = new;
		sym = sym->next;
	}
	return head;
}

static int
matches(Symbol * const sym, const char * restrict msg)
{
	if (sym == NULL)
		return *msg == 0;
	if (*msg == 0)
		return sym == NULL;
	const RuleTree * const rule = getrule(sym->num);
	if (rule->type == CHARACTER) {
		if (rule->val.ch == msg[0])
			return matches(sym->next, msg + 1);
		return 0;
	} else {
		const Sequence *seq;
		for (seq = rule->val.seq; seq != NULL; seq = seq->next) {
			Symbol * const clone = clonesymb(seq->val);
			if (clone == NULL)
				return -1;
			Symbol *tail = clone;
			while (tail->next != NULL)
				tail = tail->next;
			tail->next = sym->next;
			const int res = matches(clone, msg);
			tail->next = NULL;
			freesymbols(clone);
			if (res != 0)
				return res;
		}
		return 0;
	}
}

static Symbol *
makesymbols(const size_t size, const uintmax_t arr[size])
{
	Symbol *head = NULL, *tail = NULL;
	for (size_t i = 0; i < size; i++) {
		Symbol * const new = malloc(sizeof(Symbol));
		if (new == NULL) {
			freesymbols(head);
			return NULL;
		}
		new->num = arr[i];
		new->next = NULL;
		if (head == NULL)
			head = new;
		else
			tail->next = new;
		tail = new;
	}
	return head;
}

static bool
issequence(const Sequence * const seq,
           const size_t size,
           const uintmax_t arr[size])
{
	if (seq == NULL || seq->next != NULL)
		return false;
	const Symbol *sym = seq->val;
	size_t i = 0;
	while (sym != NULL && i < size) {
		if (sym->num != arr[i++])
			return false;
		sym = sym->next;
	}
	return sym == NULL && i == size;
}

static void
convertrule(const uintmax_t num,
            const size_t oldsize,
            const uintmax_t old[restrict oldsize],
            const size_t newsize,
            const uintmax_t new[restrict newsize])
{
	const RuleTree * const rule = getrule(num);
	if (rule == NULL) {
		fprintf(stderr, "Rule %ju not found\n", num);
		exit(EXIT_FAILURE);
	}
	if (rule->type != SEQUENCE) {
		fprintf(stderr,
		        "Rule %ju is not a disjuction of sequences\n",
		        num);
		exit(EXIT_FAILURE);
	}
	Sequence *seq = rule->val.seq;
	if (!issequence(seq, oldsize, old)) {
		fprintf(stderr, "Rule %ju does not generate", num);
		for (size_t i = 0; i < oldsize; i++)
			fprintf(stderr, " %ju", old[i]);
		fputc('\n', stderr);
		exit(EXIT_FAILURE);
	}
	if ((seq->next = malloc(sizeof(Sequence))) == NULL) {
		fputs("Could not allocate a new sequence\n", stderr);
		exit(EXIT_FAILURE);
	}
	if ((seq->next->val = makesymbols(newsize, new)) == NULL) {
		fputs("Could not fill new sequence\n", stderr);
		free(seq->next);
		exit(EXIT_FAILURE);
	}
	seq->next->next = NULL;
}

static void
convertrules(void)
{
	const uintmax_t old8[1] = { 42 };
	const uintmax_t new8[2] = { 42, 8 };
	convertrule(8, 1, old8, 2, new8);
	const uintmax_t old11[2] = { 42, 31 };
	const uintmax_t new11[3] = { 42, 11, 31 };
	convertrule(11, 2, old11, 3, new11);
}

static void
freerules(RuleTree * const set)
{
	if (set == NULL)
		return;
	freerules(set->left);
	freerules(set->right);
	if (set->type == SEQUENCE)
		freeseq(set->val.seq);
	free(set);
}

static bool
isemptyline(FILE * const in)
{
	const int c = fgetc(in);
	if (c != '\n' && c != EOF)
		ungetc(c, in);
	return c == '\n' || c == EOF;
}

static Sequence *
parsesequencerule(char *str)
{
	Sequence *seqhead = NULL, *seqtail = NULL;
	Symbol *symtail = NULL;
	bool newseq = true;
	const char *tok;
	while ((tok = strtok(str, " ")) != NULL) {
		str = NULL;
		if (newseq) {
			Sequence * const new = malloc(sizeof(Sequence));
			if (new == NULL) {
				freeseq(seqhead);
				return NULL;
			}
			new->val = NULL;
			new->next = NULL;
			if (seqhead == NULL)
				seqhead = new;
			else
				seqtail->next = new;
			seqtail = new;
			symtail = NULL;
			newseq = false;
		}
		uintmax_t num;
		if (strcmp(tok, "|") == 0) {
			if (symtail == NULL) {
				freeseq(seqhead);
				return NULL;
			}
			newseq = true;
		} else if (sscanf(tok, "%ju", &num) == 1) {
			Symbol * const new = malloc(sizeof(Symbol));
			if (new == NULL) {
				freeseq(seqhead);
				return NULL;
			}
			new->num = num;
			new->next = NULL;
			if (seqtail->val == NULL)
				seqtail->val = new;
			else
				symtail->next = new;
			symtail = new;
		} else {
			freeseq(seqhead);
			return NULL;
		}
	}
	if (newseq) {
		freeseq(seqhead);
		return NULL;
	}
	return seqhead;
}

static bool
parsegrammar(FILE * const restrict in,
             regex_t * restrict greg,
             const uintmax_t line)
{
	uintmax_t num;
	char *input;
	if (fscanf(in, "%ju: %m[^\n]", &num, &input) < 2) {
		fprintf(stderr, "Input parsing failed on line %ju\n", line);
		return false;
	}
	regmatch_t match[3];
	if (regexec(greg, input, 3, match, 0) != 0) {
		fprintf(stderr, "Line %ju doesn't match: %s\n", line, input);
		free(input);
		return false;
	}
	RuleTree * const new = malloc(sizeof(RuleTree));
	if (new == NULL) {
		fprintf(stderr, "Could not allocate rule on line %ju\n", line);
		free(input);
		return false;
	}
	new->num = num;
	new->flag = false;
	new->left = new->right = NULL;
	if (match[2].rm_so >= 0) {
		new->type = CHARACTER;
		new->val.ch = input[match[2].rm_so];
	} else {
		new->type = SEQUENCE;
		new->val.seq = parsesequencerule(input);
		if (new->val.seq == NULL) {
			fprintf(stderr, "Allocation error on line %ju\n", line);
			free(input);
			return false;
		}
	}
	free(input);
	if (addrule(new)) {
		freerules(new);
		fputs("Duplicate rules found\n", stderr);
		return false;
	}
	const int next = fgetc(in);
	if (next != '\n') {
		fprintf(stderr, "Unexpected character: %c\n", next);
		return false;
	}
	return true;
}

static bool
parsemessage(FILE * const restrict in,
             const uintmax_t line, 
             Message ** restrict tail)
{
	char *input;
	if (fscanf(in, "%m[ab]", &input) < 1) {
		fprintf(stderr, "Input failed on line %ju\n", line);
		return false;
	}
	Message * const new = malloc(sizeof(Message));
	if (new == NULL) {
		fprintf(stderr, "Allocation failed on line %ju\n", line);
		free(input);
		return false;
	}
	new->msg = input;
	new->next = NULL;
	if (msg == NULL)
		msg = new;
	else
		(*tail)->next = new;
	*tail = new;
	const int next = fgetc(in);
	if (next != '\n' && next != EOF) {
		fprintf(stderr, "Unexpected character: %c\n", next);
		return false;
	}
	return true;
}

static void
parse(FILE * const in)
{
	regex_t greg;
	int result = regcomp(&greg, GRAMMAR_REGEX, REG_EXTENDED);
	if (result != 0) {
		const size_t sz = regerror(result, &greg, NULL, 0);
		char buf[sz];
		regerror(result, &greg, buf, sz);
		fprintf(stderr, "Could not compile regex: %s\n", buf);
		exit(EXIT_FAILURE);
	}
	Message *msgtail = NULL;
	ParseState state = GRAMMAR;
	uintmax_t line = 0;
	while (!feof(in) && !ferror(in)) {
		line++;
		if (isemptyline(in)) {
			state = MESSAGES;
			continue;
		}
		switch (state) {
		case GRAMMAR:
			if (!parsegrammar(in, &greg, line)) {
				regfree(&greg);
				exit(EXIT_FAILURE);
			}
			break;
		case MESSAGES:
			if (!parsemessage(in, line, &msgtail)) {
				regfree(&greg);
				exit(EXIT_FAILURE);
			}
		}
	}
	regfree(&greg);
}

static bool
hascycle(const uintmax_t num)
{
	RuleTree * const node = getrule(num);
	if (node == NULL) {
		fprintf(stderr, "Grammar rule %ju not found\n", num);
		exit(EXIT_FAILURE);
	}
	if (node->flag)
		return true;
	if (node->type == SEQUENCE) {
		node->flag = true;
		for (Sequence *seq = node->val.seq; seq; seq = seq->next) {
			for (Symbol *sym = seq->val; sym; sym = sym->next) {
				if (hascycle(sym->num)) {
					node->flag = false;
					return true;
				}
			}
		}
		node->flag = false;
	}
	return false;
}

static uintmax_t
countmatches(void)
{
	uintmax_t count = 0;
	Symbol sym = { .num = 0, .next = NULL };
	for (const Message *m = msg; m != NULL; m = m->next) {
		const int res = matches(&sym, m->msg);
		if (res < 0) {
			fputs("Memory allocation failed\n", stderr);
			exit(EXIT_FAILURE);
		} else if (res > 0) {
			count++;
		}
	}
	return count;
}

static void
freedata(void)
{
	while (msg != NULL) {
		Message * const next = msg->next;
		free(msg->msg);
		free(msg);
		msg = next;
	}
	freerules(root);
}

int
day19(FILE * const in)
{
	if (atexit(freedata) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	parse(in);
	if (!feof(in)) {
		fputs("Errors happened while parsing puzzle input\n", stderr);
		return EXIT_FAILURE;
	}
	if (hascycle(0)) {
		fputs("The rules are cyclical\n", stderr);
		return EXIT_FAILURE;
	}
	printf("Default\t%ju\n", countmatches());
	convertrules();
	printf("Fixed\t%ju\n", countmatches());
	return EXIT_SUCCESS;
}
