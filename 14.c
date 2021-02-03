#include <errno.h>
#include <inttypes.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Linked list took ~15 s; binary tree takes ~50 ms */
struct Node {
	uint64_t addr;
	uint64_t val;
	struct Node *left;
	struct Node *right;
};

typedef struct Node Node;

static regex_t maskreg, memreg;
static Node *head;

static void
freetree(Node *node)
{
	if (node == NULL)
		return;
	freetree(node->left);
	freetree(node->right);
	free(node);
}

/* Consumes the tree; this saves a couple ms */
static uint64_t
sumtree(Node *node)
{
	if (node == NULL)
		return 0;
	uint64_t sum = node->val + sumtree(node->left) + sumtree(node->right);
	free(node->left);
	free(node->right);
	node->left = NULL;
	node->right = NULL;
	return sum;
}

static void
pregerr(const int result, const regex_t *const preg)
{
	size_t n = 8;
	for (;;) {
		char buf[n];
		if (regerror(result, preg, buf, n) >= n) {
			n *= 2;
		} else {
			fprintf(stderr, "Could not compile regex: %s\n", buf);
			return;
		}
	}
}

static void
addnode(uint64_t addr, uint64_t val)
{
	Node *node = malloc(sizeof(Node));
	if (node == NULL) {
		perror("Allocation failed");
		exit(EXIT_FAILURE);
	}
	node->addr = addr;
	node->val = val;
	node->left = NULL;
	node->right = NULL;
	if (head == NULL) {
		head = node;
		return;
	}
	Node *it = head;
	for (;;) {
		if (addr == it->addr) {
			it->val = val;
			free(node);
			return;
		} else if (addr < it->addr) {
			if (it->left == NULL) {
				it->left = node;
				return;
			} else {
				it = it->left;
			}
		} else {
			if (it->right == NULL) {
				it->right = node;
				return;
			} else {
				it = it->right;
			}
		}
	}
}

static void
floataddr(uint64_t addr, const bool *floating, uint8_t bit, uint64_t val)
{
	if (bit >= 36) {
		addnode(addr, val);
	} else if (!floating[bit]) {
		floataddr(addr, floating, bit + 1, val);
	} else {
		addr &= ~(UINT64_C(1) << bit);
		floataddr(addr, floating, bit + 1, val);
		addr |= UINT64_C(1) << bit;
		floataddr(addr, floating, bit + 1, val);
	}
}

static void
runmeminstr(char *input, const regmatch_t *match, uint64_t *mem, uint64_t mask)
{
	input[match[1].rm_eo] = 0;
	uint16_t addr;
	if (sscanf(input + match[1].rm_so, "%" SCNu16, &addr) != 1) {
		fprintf(stderr,
		        "%s doesn't fit in a 16-bit integer\n",
		        input + match[1].rm_so);
		exit(EXIT_FAILURE);
	}
	uint64_t value;
	if (sscanf(input + match[2].rm_so, "%" SCNu64, &value) != 1) {
		fprintf(stderr,
		        "%s doesn't fit in a 64-bit integer\n",
		        input + match[2].rm_so);
		exit(EXIT_FAILURE);
	}
	uint64_t modaddr = addr;
	uint64_t modval = value;
	bool floating[36] = { false };
	for (uint64_t i = 0; i < 36; i++) {
		if (mask % 3 == 2) {
			modval |= UINT64_C(1) << i;
			modaddr |= UINT64_C(1) << i;
		} else if (mask % 3 == 1) {
			modval &= ~(UINT64_C(1) << i);
		} else {
			floating[i] = true;
		}
		mask /= 3;
	}
	floataddr(modaddr, floating, 0, value);
	mem[addr] = modval;
}

static void
freedata(void)
{
	regfree(&maskreg);
	regfree(&memreg);
	freetree(head);
}

int
day14(void)
{
	if (atexit(freedata) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	int res = regcomp(&maskreg, "^mask = [X0-9]{36}$", REG_EXTENDED);
	if (res != 0) {
		pregerr(res, &maskreg);
		return EXIT_FAILURE;
	}
	res = regcomp(&memreg, "^mem\\[([0-9]+)\\] = ([0-9]+)$", REG_EXTENDED);
	if (res != 0) {
		pregerr(res, &memreg);
		return EXIT_FAILURE;
	}
	uint64_t mask = 0;
	uint64_t mem[65536] = { 0 };
	char input[48];
	while ((res = scanf("%47[^\n]%*1[\n]", input)) == 1) {
		regmatch_t match[3];
		if (regexec(&maskreg, input, 0, NULL, 0) == 0) {
			mask = 0;
			for (uint8_t i = 0; i < 36; i++) {
				mask *= 3;
				if ((input[7 + i] >> 1) == ('0' >> 1))
					mask += input[7 + i] - '0' + 1;
			}
		} else if (regexec(&memreg, input, 3, match, 0) == 0) {
			runmeminstr(input, match, mem, mask);
		} else {
			fprintf(stderr, "Bad input on line: \"%s\"\n", input);
			return EXIT_FAILURE;
		}
	}
	if (res != EOF) {
		if (errno != 0)
			perror("Input failed");
		else
			fputs("Bad input format\n", stderr);
		return EXIT_FAILURE;
	}
	uint64_t sum = 0;
	for (uint32_t i = 0; i < 65536; i++) {
		if (mem[i] > UINT64_MAX - sum)
			fputs("Overflow detected\n", stderr);
		sum += mem[i];
	}
	printf("Ver 1\t%" PRIu64 "\n", sum);
	printf("Ver 2\t%" PRIu64 "\n", sumtree(head));
	return EXIT_SUCCESS;
}

