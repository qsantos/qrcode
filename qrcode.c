#include "qrcode.h"

#include <stdlib.h>

#include "modules.h"

static void         check_finder(scanner_t* scanner, size_t i, size_t j);
static unsigned int get_bits    (scanner_t* scanner, size_t n);

static void check_finder(scanner_t* scanner, size_t i, size_t j)
{
	static const char mask[7][7] =
	{
		{ 1,1,1,1,1,1,1, },
		{ 1,0,0,0,0,0,1, },
		{ 1,0,1,1,1,0,1, },
		{ 1,0,1,1,1,0,1, },
		{ 1,0,1,1,1,0,1, },
		{ 1,0,0,0,0,0,1, },
		{ 1,1,1,1,1,1,1, },
	};
	for (size_t ki = 0; ki < 7; ki++)
	for (size_t kj = 0; kj < 7; kj++)
		if (P(i+ki,j+kj) != mask[ki][kj])
		{
			fprintf(stderr, "Invalid finder at (%zu,%zu)\n", i, j);
			exit(1);
		}
}
static unsigned int get_bits(scanner_t* scanner, size_t n)
{
	unsigned int res = 0;
	while (n)
	{
		if (!scanner->buf_avail)
		{
			scanner->buf = get_codeword(scanner);
			scanner->buf_avail = 8;
		}
		res = res*2 + ((scanner->buf >> (scanner->buf_avail-1))& 1);
		scanner->buf_avail--;
		n--;
	}
	return res;
}
void qrc_decode(scanner_t* scanner)
{
	// size
	size_t s = scanner->s;

	// finders
	check_finder(scanner, 0, 0);
	check_finder(scanner, s-7, 0);
	check_finder(scanner, 0, s-7);

	// version
	size_t v = s - 17;
	if (v % 4 != 0)
	{
		fprintf(stderr, "Invalid image size\n");
		exit(1);
	}
	v /= 4;
	if (!(1 <= v && v <= 40))
	{
		fprintf(stderr, "Unsupported version '%zu'\n", v);
		exit(1);
	}
	scanner->v = v;

	// mask
	scanner->m = 4*P(s-3,8) + 2*P(s-4,8) + P(s-5,8);

	// error correction level
	int c = (!P(s-1,8))*2 + (!P(s-2,8));

	static const int blocks[160][7] =
	{
#include "blocksizes.h"
	};

	// initialize block information
	const int* b = blocks[4*(v-1) + c];
	scanner->blocks = b;
	// count all the blocks
	{
		int n = 0;
		for (size_t i = 0; b[i]; i+=3)
			n += b[i];
		scanner->block_count = n;
	}
	scanner->block_cur = 0;
	scanner->block_dataw = b[2];
	scanner->cur_word = 0;

	// initialize geting
	scanner->i = s-1;
	scanner->j = s-1;
	scanner->buf = 0;
	scanner->buf_avail = 0;

	while (1)
	{
		byte enc = get_bits(scanner, 4);

		if (enc == 0) // END
		{
			break;
		}
		else if (enc == 1) // numeric
		{
			size_t lenbits = 10;
			if (v >= 27) lenbits = 14;
			else if (v >= 10) lenbits = 12;
			size_t len = get_bits(scanner, lenbits);

			for (; len>=3; len-=3)
			{
				unsigned int c = get_bits(scanner, 10);
				printf("%u", (c/100) % 10);
				printf("%u", (c/ 10) % 10);
				printf("%u", (c/  1) % 10);
			}
			if (len == 2)
			{
				unsigned int c = get_bits(scanner, 7);
				printf("%u", c/10);
				printf("%u", c%10);
			}
			else if (len == 1)
			{
				unsigned int c = get_bits(scanner, 4);
				printf("%u", c);
			}
		}
		else if (enc == 2) // alphanumeric
		{
			size_t lenbits = 9;
			if (v >= 27) lenbits = 13;
			else if (v >= 10) lenbits = 11;
			size_t len = get_bits(scanner, lenbits);

			static const char* map = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%%*+-./:";
			for (; len >= 2; len-=2)
			{
				unsigned int c = get_bits(scanner, 11);
				printf("%c", map[c/45]);
				printf("%c", map[c%45]);
			}
			if (len == 1)
			{
				unsigned int c = get_bits(scanner, 6);
				printf("%c", map[c]);
			}
		}
		else if (enc == 4) // Shift JIS
		{
			size_t lenbits = 8;
			if (v >= 10) lenbits = 16;
			size_t len = get_bits(scanner, lenbits);

			for (size_t i = 0; i < len; i++)
			{
				byte c = get_bits(scanner, 8);
				printf("%c", c);
			}
		}
		else
		{
			fprintf(stderr, "Unsupported encoding '%u'\n", enc);
			exit(1);
		}
	}
	printf("\n");
}
