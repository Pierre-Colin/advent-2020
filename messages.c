/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRINT_ERR(s) \
	if (errno != 0) \
		perror(s); \
	else \
		fputs(s "\n", stderr);

/* Upper bound to how many digits a given type may hold */
#define DIGITS(T) (CHAR_BIT * 10 * sizeof(T) / (9 * sizeof(char)))

#define GRAMMAR_REGEX "[0-9]+( \\| [0-9]+| [0-9]+)*|\"([ab])\""

typedef enum {
	GRAMMAR,
	MESSAGES
} ParseState;

struct Message {
	char *msg;
	struct Message *next;
};

typedef struct Message Message;

typedef enum {
	CHARACTER,
	SEQUENCE
} RuleType;

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
	struct RuleTree *left;
	struct RuleTree *right;
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
		Symbol *next = sym->next;
		free(sym);
		sym = next;
	}
}

static void
freeseq(Sequence *seq)
{
	while (seq != NULL) {
		Sequence *next = seq->next;
		freesymbols(seq->val);
		free(seq);
		seq = next;
	}
}

static Symbol *
clonesymb(const Symbol *restrict sym)
{
	Symbol *head = NULL, *tail = NULL;
	errno = 0;
	while (sym != NULL) {
		Symbol *const new = malloc(sizeof(Symbol));
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
matches(Symbol *const sym, const char *restrict msg)
{
	if (sym == NULL)
		return *msg == 0;
	if (*msg == 0)
		return sym == NULL;
	const RuleTree *rule = getrule(sym->num);
	if (rule->type == CHARACTER) {
		if (rule->val.ch == msg[0])
			return matches(sym->next, msg + 1);
		return 0;
	} else {
		for (Sequence *seq = rule->val.seq; seq; seq = seq->next) {
			Symbol *clone = clonesymb(seq->val);
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
	errno = 0;
	for (size_t i = 0; i < size; i++) {
		Symbol *new = malloc(sizeof(Symbol));
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
issequence(const Sequence *const seq, size_t size, const uintmax_t arr[size])
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
            const uintmax_t old[oldsize],
            const size_t newsize,
            const uintmax_t new[newsize])
{
	const RuleTree *const rule = getrule(num);
	if (rule == NULL) {
		fprintf(stderr, "Rule %" PRIuMAX " not found\n", num);
		exit(EXIT_FAILURE);
	}
	if (rule->type != SEQUENCE) {
		fprintf(stderr,
		        "Rule %" PRIuMAX " is not a disjuction of sequences\n",
		        num);
		exit(EXIT_FAILURE);
	}
	Sequence *seq = rule->val.seq;
	if (!issequence(seq, oldsize, old)) {
		fprintf(stderr, "Rule %" PRIuMAX " does not generate", num);
		for (size_t i = 0; i < oldsize; i++)
			fprintf(stderr, " %" PRIuMAX, old[i]);
		fputc('\n', stderr);
		exit(EXIT_FAILURE);
	}
	if ((seq->next = malloc(sizeof(Sequence))) == NULL) {
		PRINT_ERR("Could not allocate new sequence");
		exit(EXIT_FAILURE);
	}
	if ((seq->next->val = makesymbols(newsize, new)) == NULL) {
		PRINT_ERR("Could not fill new sequence");
		free(seq->next);
		exit(EXIT_FAILURE);
	}
	seq->next->next = NULL;
}

static void
convertrules(void)
{
	errno = 0;
	uintmax_t old8[1] = { 42 };
	uintmax_t new8[2] = { 42, 8 };
	convertrule(8, 1, old8, 2, new8);
	uintmax_t old11[2] = { 42, 31 };
	uintmax_t new11[3] = { 42, 11, 31 };
	convertrule(11, 2, old11, 3, new11);
}

static void
freerules(RuleTree *set)
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
isemptyline(void)
{
	const int c = getchar();
	if (c != '\n' && c != EOF)
		ungetc(c, stdin);
	return c == '\n' || c == EOF;
}

static void
printinputfailed(const uintmax_t line)
{
	if (errno != 0) {
		const size_t sz = 22 + DIGITS(uintmax_t);
		char buf[sz];
		sprintf(buf, "Input failed on line %" PRIuMAX, line);
		perror(buf);
	} else {
		fprintf(stderr, "Bad input on line %" PRIuMAX "\n", line);
	}
}

static void
printallocationerror(const uintmax_t line)
{
	if (errno != 0) {
		const int temp = errno;
		const size_t sz = 27 + DIGITS(uintmax_t);
		char buf[sz];
		sprintf(buf, "Allocation failed on line %" PRIuMAX, line);
		errno = temp;
		perror(buf);
	} else {
		fprintf(stderr,
		        "Allocation failed on line " "%" PRIuMAX "\n",
		        line);
	}
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
			errno = 0;
			Sequence *new = malloc(sizeof(Sequence));
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
		} else if (sscanf(tok, "%" SCNuMAX, &num) == 1) {
			errno = 0;
			Symbol *new = malloc(sizeof(Symbol));
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
parsegrammar(regex_t *greg, const uintmax_t line)
{
	uintmax_t num;
	char *input;
	errno = 0;
	if (scanf("%" SCNuMAX ": %m[^\n]%*1[\n]", &num, &input) == 2) {
		regmatch_t match[3];
		if (regexec(greg, input, 3, match, 0) != 0) {
			fprintf(stderr,
			        "Line %" PRIuMAX " doesn't match: %s\n",
			        line,
			        input);
			free(input);
			return false;
		}
		errno = 0;
		RuleTree *new = malloc(sizeof(RuleTree));
		if (new == NULL) {
			printallocationerror(line);
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
				printallocationerror(line);
				free(input);
				return false;
			}
		}
		free(input);
		if (addrule(new)) {
			freerules(new);
			fputs("Duplicate rule\n", stderr);
			return false;
		}
		return true;
	} else {
		printinputfailed(line);
		return false;
	}
}

static bool
parsemessage(const uintmax_t line, Message **tail)
{
	char *input;
	errno = 0;
	if (scanf("%m[ab]%*1[\n]", &input) == 1) {
		errno = 0;
		Message *new = malloc(sizeof(Message));
		if (new == NULL) {
			if (errno != 0) {
				const size_t sz = 35 + DIGITS(uintmax_t);
				char buf[sz];
				sprintf(buf,
				        "Message allocation failed on line "
				        "%" PRIuMAX,
				        line);
				perror(buf);
			} else {
				fprintf(stderr,
				        "Message allocation failed on line "
				        "%" PRIuMAX "\n",
				        line);
			}
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
	} else {
		printinputfailed(line);
		return false;
	}
	return true;
}

static void
parse(void)
{
	regex_t greg;
	int result = regcomp(&greg, GRAMMAR_REGEX, REG_EXTENDED);
	if (result != 0) {
		size_t size = 2;
		for (;;) {
			char buf[size];
			if (regerror(result, &greg, buf, size) < size) {
				fprintf(stderr,
				        "Could not compile regex: %s\n",
				        buf);
				exit(EXIT_FAILURE);
			}
			size *= 2;
		}
	}
	Message *msgtail = NULL;
	ParseState state = GRAMMAR;
	uintmax_t line = 0;
	while (!feof(stdin) && !ferror(stdin)) {
		line++;
		if (isemptyline()) {
			state = MESSAGES;
			continue;
		}
		switch (state) {
		case GRAMMAR:
			if (!parsegrammar(&greg, line)) {
				regfree(&greg);
				exit(EXIT_FAILURE);
			}
			break;
		case MESSAGES:
			if (!parsemessage(line, &msgtail)) {
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
	RuleTree *node = getrule(num);
	if (node == NULL) {
		fprintf(stderr, "Rule %" PRIuMAX " not found\n", num);
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

static void
findcycle(void)
{
	if (hascycle(0)) {
		fputs("The rules are cyclical\n", stderr);
		exit(EXIT_FAILURE);
	}
}

static uintmax_t
countmatches(void)
{
	uintmax_t count = 0;
	Symbol sym = { .num = 0, .next = NULL };
	for (const Message *m = msg; m != NULL; m = m->next) {
		const int res = matches(&sym, m->msg);
		if (res < 0) {
			PRINT_ERR("Failed allocation");
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
	Message *m = msg;
	while (m != NULL) {
		Message *const next = m->next;
		free(m->msg);
		free(m);
		m = next;
	}
	freerules(root);
}

int
day19(void)
{
	if (atexit(freedata) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	parse();
	if (!feof(stdin)) {
		fputs("Errors happened while parsing puzzle input\n", stderr);
		return EXIT_FAILURE;
	}
	findcycle();
	printf("Default\t%" PRIuMAX "\n", countmatches());
	convertrules();
	printf("Fixed\t%" PRIuMAX "\n", countmatches());
	return EXIT_SUCCESS;
}

