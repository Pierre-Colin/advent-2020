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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Upper bound to how many digits a given type may hold */
#define DIGITS(T) (CHAR_BIT * 10 * sizeof(T) / (9 * sizeof(char)))

struct Tile {
	uintmax_t num;
	bool *data;
	struct Tile *next;
};

typedef struct Tile Tile;

typedef struct {
	const Tile *tile;
	bool flip;
	uint_least8_t rot;
} Slot;

static size_t tilesz = 0;
static Tile *head = NULL;
static size_t jigsawsz = 0;
static size_t imagesz = 0;

static const bool monster[3][20] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
	{ 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1 },
	{ 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0 }
};

static void
printlineerror(const char *const str, const uintmax_t line)
{
	if (errno != 0) {
		char buf[strlen(str) + 10 + DIGITS(uintmax_t)];
		sprintf(buf, "%s on line %" PRIuMAX, str, line);
		perror(buf);
	} else {
		fprintf(stderr, "%s on line %" PRIuMAX "\n", str, line);
	}
}

static bool
hastile(const uintmax_t num)
{
	for (const Tile *tile = head; tile != NULL; tile = tile->next) {
		if (tile->num == num)
			return true;
	}
	return false;
}

static uintmax_t
parselabel(const uintmax_t line)
{
	char newline;
	uintmax_t tilenum;
	if (scanf("Tile %" SCNuMAX ":%1c", &tilenum, &newline) < 2) {
		printlineerror("Input failed", line);
		exit(EXIT_FAILURE);
	}
	if (newline != '\n') {
		fprintf(stderr,
		        "Mising line break on line %" PRIuMAX "\n",
		        line);
		exit(EXIT_FAILURE);
	}
	return tilenum;
}

static void
expectnewline(const uintmax_t line, void *const ptr)
{
	char end = getchar();
	if (end != EOF && end != '\n') {
		fprintf(stderr,
			"Expected new line on line %" PRIuMAX "\n",
			line);
		free(ptr);
		exit(EXIT_FAILURE);
	} else if (ferror(stdin)) {
		printlineerror("Input failed", line);
		free(ptr);
		exit(EXIT_FAILURE);
	}
}

static void
filltileline(const uintmax_t line,
              const char *restrict const format,
              bool *restrict const tile,
              const size_t l)
{
	char buf[tilesz + 1];
	char newline;
	const int result = scanf(format, buf, &newline);
	if ((result < 2 && !feof(stdin)) || result < 1) {
		printlineerror("Input failed", line);
		free(tile);
		exit(EXIT_FAILURE);
	} else if (newline != '\n' && !feof(stdin)) {
		fprintf(stderr,
		        "Missing line break on line %" PRIuMAX "\n",
		        line);
		free(tile);
		exit(EXIT_FAILURE);
	}
	for (size_t i = 0; i < tilesz; i++) {
		if (buf[i] == 0) {
			fprintf(stderr,
			        "Inconsistent width on line %" PRIuMAX "\n",
			        line);
			free(tile);
			exit(EXIT_FAILURE);
		}
		tile[l * tilesz + i] = buf[i] == '#';
	}
}

static bool *
parsetile(uintmax_t *const line)
{
	static char format[8 + DIGITS(size_t)];
	bool *tile = NULL;
	if (tilesz == 0) {
		char *input;
		char newline;
		if (scanf("%m[.#]%1c", &input, &newline) < 2) {
			printlineerror("Input failed", *line);
			exit(EXIT_FAILURE);
		} else if (newline != '\n') {
			fprintf(stderr,
			        "Bad input format on line %" PRIuMAX "\n",
			        *line);
			free(input);
			exit(EXIT_FAILURE);
		}
		tilesz = strlen(input);
		if ((tile = malloc(tilesz * tilesz * sizeof(bool))) == NULL) {
			printlineerror("Allocation failed", *line);
			free(input);
			exit(EXIT_FAILURE);
		}
		for (size_t i = 0; i < tilesz; i++)
			tile[i] = input[i] == '#';
		free(input);
		(*line)++;
		sprintf(format, "%%%zu[#.]%%c", tilesz);
	} else {
		if ((tile = malloc(tilesz * tilesz * sizeof(bool))) == NULL) {
			printlineerror("Allocation failed", *line);
			exit(EXIT_FAILURE);
		}
		filltileline((*line)++, format, tile, 0);
	}
	for (size_t l = 1; l < tilesz; l++)
		filltileline((*line)++, format, tile, l);
	expectnewline(*line, tile);
	return tile;
}

static bool
keepparsing(void)
{
	const char c = getchar();
	if (c != EOF)
		ungetc(c, stdin);
	return c != EOF;
}

static void
parse(void)
{
	Tile *tail = NULL, *tile;
	uintmax_t line = 1, num = 0;
	while (keepparsing()) {
		errno = 0;
		num = parselabel(line++);
		if (hastile(num)) {
			fprintf(stderr,
			        "Tile %" PRIuMAX " appears twice\n",
			        num);
			exit(EXIT_FAILURE);
		}
		bool *const tiledata = parsetile(&line);
		if ((tile = malloc(sizeof(Tile))) == NULL) {
			printlineerror("Allocation failed", line);
			free(tile);
			exit(EXIT_FAILURE);
		}
		tile->num = num;
		tile->data = tiledata;
		tile->next = NULL;
		if (head == NULL)
			head = tile;
		else
			tail->next = tile;
		tail = tile;
		line++;
	}
}

static uintmax_t
isqrt(const uintmax_t n)
{
	if (n <= 1)
		return n;
	uintmax_t x = n / 2;
	uintmax_t y = (x + n / x) / 2;
	while (y < x) {
		x = y;
		y = (x + n / x) / 2;
	}
	return x;
}

static void
checkperfectsquare(void)
{
	uintmax_t num = 0;
	for (const Tile *tile = head; tile != NULL; tile = tile->next)
		num++;
	jigsawsz = isqrt(num);
	if (jigsawsz * jigsawsz != num) {
		fprintf(stderr,
		        "Number of tiles (%" PRIuMAX ") is not a square\n",
		        num);
		exit(EXIT_FAILURE);
	}
}

static void
rotatebuf(const size_t sz, bool buf[sz][sz])
{
	for (size_t r = 0; 2 * r < sz; r++) {
		for (size_t c = 0; 2 * c < sz; c++) {
			const size_t invr = sz - 1 - r;
			const size_t invc = sz - 1 - c;
			const bool temp = buf[r][c];
			buf[r][c] = buf[c][invr];
			buf[c][invr] = buf[invr][invc];
			buf[invr][invc] = buf[invc][r];
			buf[invc][r] = temp;
		}
	}
	if (sz % 2 == 0)
		return;
	for (size_t r = 0; 2 * r < sz; r++) {
		const size_t h = sz / 2;
		const bool temp = buf[r][h];
		buf[r][h] = buf[h][sz - 1 - r];
		buf[h][sz - 1 - r] = buf[sz - 1 - r][h];
		buf[sz - 1 - r][h] = buf[h][r];
		buf[h][r] = temp;
	}
}

static void
flipbuf(const size_t sz, bool buf[sz][sz])
{
	for (size_t r = 0; r < sz; r++) {
		for (size_t c = 0; 2 * c < sz; c++) {
			const bool temp = buf[r][c];
			buf[r][c] = buf[r][sz - 1 - c];
			buf[r][sz - 1 - c] = temp;
		}
	}
}

static bool
nexttry(bool tileimg[tilesz][tilesz], Slot *const s)
{
	if (s->flip) {
		if (s->rot == 3)
			return false;
		s->rot++;
		rotatebuf(tilesz, tileimg);
		return true;
	} else {
		rotatebuf(tilesz, tileimg);
		if (s->rot == 3) {
			s->flip = true;
			s->rot = 0;
			flipbuf(tilesz, tileimg);
		} else {
			s->rot++;
		}
		return true;
	}
}

static bool
alreadyused(const Slot jigsaw[jigsawsz][jigsawsz],
            const size_t y,
            const size_t x,
            const Tile *const tile)
{
	for (size_t r = 0; r < y; r++) {
		for (size_t c = 0; c < jigsawsz; c++) {
			if (jigsaw[r][c].tile == tile)
				return true;
		}
	}
	for (size_t c = 0; c < x; c++) {
		if (jigsaw[y][c].tile == tile)
			return true;
	}
	return false;
}

static void
applyslot(bool buf[tilesz][tilesz], const Slot *const slot)
{
	for (size_t r = 0; r < tilesz; r++) {
		for (size_t c = 0; c < tilesz; c++)
			buf[r][c] = slot->tile->data[r * tilesz + c];
	}
	if (slot->flip)
		flipbuf(tilesz, buf);
	for (uint_least8_t t = 0; t < slot->rot; t++)
		rotatebuf(tilesz, buf);
}

static void
filldown(bool down[tilesz], const Slot *restrict const slot)
{
	bool tile[tilesz][tilesz];
	applyslot(tile, slot);
	for (size_t i = 0; i < tilesz; i++)
		down[i] = tile[tilesz - 1][i];
}

static void
fillright(bool right[tilesz], const Slot *restrict const slot)
{
	bool tile[tilesz][tilesz];
	applyslot(tile, slot);
	for (size_t i = 0; i < tilesz; i++)
		right[i] = tile[i][tilesz - 1];
}

static bool
lastfits(const bool tile[tilesz][tilesz],
         const size_t y,
         const size_t x,
         const bool up[tilesz],
         const bool left[tilesz])
{
	for (size_t i = 0; i < tilesz; i++) {
		if (y > 0 && tile[0][i] != up[i])
			return false;
		if (x > 0 && tile[i][0] != left[i])
			return false;
	}
	return true;
}

static bool
backtrack(Slot jigsaw[jigsawsz][jigsawsz], const size_t y, const size_t x)
{
	bool up[tilesz], left[tilesz];
	if (y > 0)
		filldown(up, &jigsaw[y - 1][x]);
	if (x > 0)
		fillright(left, &jigsaw[y][x - 1]);
	for (const Tile *tile = head; tile != NULL; tile = tile->next) {
		if (alreadyused(jigsaw, y, x, tile))
			continue;
		jigsaw[y][x].tile = tile;
		jigsaw[y][x].flip = false;
		jigsaw[y][x].rot = 0;
		bool tileimg[tilesz][tilesz];
		applyslot(tileimg, &jigsaw[y][x]);
		do {
			if (lastfits(tileimg, y, x, up, left)) {
				if (x + 1 == jigsawsz) {
					if (y + 1 == jigsawsz)
						return true;
					else if (backtrack(jigsaw, y + 1, 0))
						return true;
				} else if (backtrack(jigsaw, y, x + 1)) {
					return true;
				}
			}
		} while (nexttry(tileimg, &jigsaw[y][x]));
	}
	return false;
}

static void
trymultiply(uintmax_t *restrict const p, const uintmax_t x)
{
	if (*p >= UINTMAX_MAX / x) {
		fprintf(stderr,
		        "%" PRIuMAX " * %" PRIuMAX " causes wraparound\n",
		        *p,
		        x);
		exit(EXIT_FAILURE);
	}
	*p *= x;
}

static uintmax_t
prodcorners(const Slot jigsaw[jigsawsz][jigsawsz])
{
	uintmax_t p = jigsaw[0][0].tile->num;
	trymultiply(&p, jigsaw[jigsawsz - 1][0].tile->num);
	trymultiply(&p, jigsaw[0][jigsawsz - 1].tile->num);
	trymultiply(&p, jigsaw[jigsawsz - 1][jigsawsz - 1].tile->num);
	return p;
}

static void
fillimage(bool image[imagesz][imagesz], const Slot jigsaw[jigsawsz][jigsawsz])
{
	for (size_t jy = 0; jy < jigsawsz; jy++) {
		for (size_t jx = 0; jx < jigsawsz; jx++) {
			bool tile[tilesz][tilesz];
			applyslot(tile, &jigsaw[jy][jx]);
			for (size_t ty = 0; ty + 2 < tilesz; ty++) {
				for (size_t tx = 0; tx + 2 < tilesz; tx++) {
					size_t iy = jy * (tilesz - 2) + ty;
					size_t ix = jx * (tilesz - 2) + tx;
					image[iy][ix] = tile[ty + 1][tx + 1];
				}
			}
		}
	}
}

static bool
ismonster(const bool image[imagesz][imagesz], const size_t y, const size_t x)
{
	for (size_t dy = 0; dy < 3; dy++) {
		for (size_t dx = 0; dx < 20; dx++) {
			if (monster[dy][dx] && !image[y + dy][x + dx])
				return false;
		}
	}
	return true;
}

static bool
findmonsters(bool image[imagesz][imagesz])
{
	for (size_t y = 0; y + 2 < imagesz; y++) {
		for (size_t x = 0; x + 19 < imagesz; x++) {
			if (ismonster(image, y, x))
				return true;
		}
	}
	return false;
}

static bool
fitformonsters(bool image[imagesz][imagesz])
{
	for (uint_fast8_t t = 0; t < 3; t++) {
		if (findmonsters(image))
			return true;
		rotatebuf(imagesz, image);
	}
	if (findmonsters(image))
		return true;
	flipbuf(imagesz, image);
	for (uint_fast8_t t = 0; t < 3; t++) {
		if (findmonsters(image))
			return true;
		rotatebuf(imagesz, image);
	}
	return findmonsters(image);
}

static bool
isinmonster(const bool image[imagesz][imagesz], const size_t y, const size_t x)
{
	if (!image[y][x])
		return false;
	for (size_t dy = 0; dy < 3; dy++) {
		if (y < dy || y - dy >= imagesz - 2)
			continue;
		for (size_t dx = 0; dx < 20; dx++) {
			if (x < dx || x - dx >= imagesz - 19)
				continue;
			if (monster[dy][dx] && ismonster(image, y - dy, x - dx))
				return true;
		}
	}
	return false;
}

static uintmax_t
roughness(const bool image[imagesz][imagesz])
{
	uintmax_t count = 0;
	for (size_t y = 0; y < imagesz; y++) {
		for (size_t x = 0; x < imagesz; x++)
			count += image[y][x] && !isinmonster(image, y, x);
	}
	return count;
}

static void
freepieces(void)
{
	while (head != NULL) {
		Tile *const next = head->next;
		free(head->data);
		free(head);
		head = next;
	}
}

int
day20(void)
{
	if (atexit(freepieces) != 0)
		fputs("Call to `atexit` failed; memory may leak\n", stderr);
	parse();
	if (!feof(stdin))
		return EXIT_FAILURE;
	checkperfectsquare();
	Slot jigsaw[jigsawsz][jigsawsz];
	if (!backtrack(jigsaw, 0, 0)) {
		fputs("No solution to the jigsaw was found\n", stderr);
		return EXIT_FAILURE;
	}
	printf("Corners\t%" PRIuMAX "\n", prodcorners(jigsaw));
	if (tilesz <= 2) {
		fprintf(stderr,
		        "Tile size %zu is too small for part 2\n",
		        tilesz);
		return EXIT_FAILURE;
	}
	imagesz = jigsawsz * (tilesz - 2);
	bool image[imagesz][imagesz];
	fillimage(image, jigsaw);
	if (!fitformonsters(image)) {
		fputs("No sea monsters were found despite rotating\n", stderr);
		return EXIT_FAILURE;
	}
	printf("Rough\t%" PRIuMAX "\n", roughness(image));
	return EXIT_SUCCESS;
}

