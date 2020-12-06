#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void
updatetotals(const uint32_t *const anyone,
             uint64_t *const atotal,
             const uint32_t *const everyone,
             uint64_t *const etotal)
{
	for (uint8_t i = 0; i < 26; i++) {
		if (*anyone & (1u << i))
			(*atotal)++;
		if (*everyone & (1u << i))
			(*etotal)++;
	}
}

int
day06(void)
{
	uint64_t atotal = 0;
	uint64_t etotal = 0;
	uint32_t anyone = 0;
	uint32_t everyone = 0x03ffffff;
	char input[27];
	int scanres;
	while ((scanres = scanf("%26[a-z]%*1[\n]", input)) != EOF) {
		if (scanres < 1) {
			if (errno != 0) {
				perror("Failed to parse input");
				return EXIT_FAILURE;
			} else if (getchar() == '\n') {
				updatetotals(&anyone,
				             &atotal,
				             &everyone,
				             &etotal);
				anyone = 0;
				everyone = 0x03ffffff;
			} else {
				fputs("Bad input format\n", stderr);
				return EXIT_FAILURE;
			}
		} else {
			uint32_t me = 0;
			for (const char *c = input; *c != 0; c++)
				me |= 1u << (*c - 'a');
			anyone |= me;
			everyone = 0x03ffffff & (everyone & me);
		}
	}
	updatetotals(&anyone, &atotal, &everyone, &etotal);
	printf("Any\t%" PRIu64 "\nEvery\t%" PRIu64 "\n", atotal, etotal);
	return EXIT_SUCCESS;
}
