#include "qrcode.h"

#include <stdlib.h>

typedef struct scanner scanner_t;

struct scanner
{
	size_t   s; // size
	int      v; // version
	color_t* d; // data
	int      m; // mask

	// block splitting
	const int* blocks; // block information
	int n_blocks;      // total number of blocks
	int cur_block;     // current block
	int cur_words;     // number of its data words
	int cur_word;      // current word

	// current bit
	size_t i;
	size_t j;

	// codeword buffer
	byte   buf;
	size_t buf_avail;
};

#define P(I,J) (scanner->d[(I)*scanner->s + (J)].r != 0 ? 1 : 0)
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
static int is_forbidden(scanner_t* scanner, size_t i, size_t j)
{
	size_t s = scanner->s;

	// finders and format information
	if (i <= 8   && j <= 8) return 1; // top-left
	if (i <= 8 && j >= s-8) return 1; // top-right
	if (j <= 8 && i >= s-8) return 1; // bottom-left

	// version information
	if (i < 6 && j >= s-11) return 1; // top-right
	if (j < 6 && i >= s-11) return 1; // bottom-left

	// timings
	if (i == 6) return 1;
	if (j == 6) return 1;

	// alignments
	if (i <= 8 && j >= s-10) return 0;
	if (j <= 8 && i >= s-10) return 0;
	static const size_t aligns[][8] =
	{
		{ 0, },
		{ 0, },
		{ 6,18,0 },
		{ 6,22,0 },
		{ 6,26,0 },
		{ 6,30,0 },
		{ 6,34,0 },
		{ 6,22,38,0 },
		{ 6,24,42,0 },
		{ 6,26,46,0 },
		{ 6,28,50,0 },
		{ 6,30,54,0 },
		{ 6,32,58,0 },
		{ 6,34,62,0 },
		{ 6,26,46,66,0 },
		{ 6,26,48,70,0 },
		{ 6,26,50,74,0 },
		{ 6,30,54,78,0 },
		{ 6,30,56,82,0 },
		{ 6,30,58,86,0 },
		{ 6,34,82,90,0 },
		{ 6,28,50,72, 94,0 },
		{ 6,26,50,74, 98,0 },
		{ 6,30,54,78,102,0 },
		{ 6,28,54,80,106,0 },
		{ 6,32,58,84,110,0 },
		{ 6,30,58,86,114,0 },
		{ 6,34,62,90,118,0 },
		{ 6,26,50,74, 98,122,0 },
		{ 6,30,54,78,102,126,0 },
		{ 6,26,52,78,104,130,0 },
		{ 6,30,56,82,108,134,0 },
		{ 6,34,60,86,112,138,0 },
		{ 6,30,58,86,114,142,0 },
		{ 6,34,62,90,118,146,0 },
		{ 6,30,54,78,102,126,150,0 },
		{ 6,24,50,76,102,128,154,0 },
		{ 6,28,54,80,106,132,158,0 },
		{ 6,32,58,84,110,136,162,0 },
		{ 6,26,54,82,110,138,166,0 },
		{ 6,30,58,86,114,142,170,0 },
	};
	int coll_x = 0;
	for (const size_t* a = aligns[scanner->v]; *a && !coll_x; a++)
		coll_x = (*a-2 <= i && i <= *a+2);
	int coll_y = 0;
	for (const size_t* a = aligns[scanner->v]; *a && !coll_y; a++)
		coll_y = (*a-2 <= j && j <= *a+2);

	return coll_x && coll_y;
}
static byte read_bit(scanner_t* scanner)
{
	size_t i = scanner->i;
	size_t j = scanner->j;

	// mask
	byte res = P(i,j);
	switch (scanner->m)
	{
	case 0: res ^= 0 == (i*j)%2 + (i*j)%3;   break;
	case 1: res ^= 0 == (i/2+j/3)%2;         break;
	case 2: res ^= 0 == ((i*j)%3+(i+j)%2)%2; break;
	case 3: res ^= 0 == ((i*j)%2+(i*j)%3)%2; break;
	case 4: res ^= 0 == i%2;                 break;
	case 5: res ^= 0 == (i+j)%2;             break;
	case 6: res ^= 0 == (i+j)%3;             break;
	case 7: res ^= 0 == j%3;                 break;
	default:
		exit(1);
	}

	// next bit
	do
	{
		if ((j/2) % 2 == 0)
		{
			if (j % 2 == 0)
			{
				j--;
			}
			else if (i >= scanner->s-1)
			{
				j--;
			}
			else
			{
				i++;
				j++;
			}
		}
		else
		{
			if (j % 2 == 0)
			{
				j--;
			}
			else if (i <= 0)
			{
				j--;
			}
			else
			{
				i--;
				j++;
			}
		}
	}
	while (is_forbidden(scanner,i,j));

	scanner->i = i;
	scanner->j = j;

	return res;
}
static byte read_codeword(scanner_t* scanner)
{
	// read next word
	byte res = 0;
	for (size_t i = 0; i < 8; i++)
	{
		res *= 2;
		res += read_bit(scanner);
	}

	// if at the end of a block, get back to the start of the next
	if (++scanner->cur_word >= scanner->cur_words)
	{
		const int* b = scanner->blocks;

		// find the current block
		size_t i = 0;
		scanner->cur_block++;
		int n = scanner->cur_block - b[0];
		while (n >= 0)
		{
			i += 3;
			n -= b[i];
		}

		// rewind
		scanner->i = scanner->s-1;
		scanner->j = scanner->s-1;

		// set info
		scanner->cur_words = b[i+2];
		scanner->cur_word = 0;

		// skip the previous blocks
		for (int i = 0; i < scanner->cur_block; i++)
			for (size_t j = 0; j < 8; j++)
				read_bit(scanner);
	}
	// otherwise, skip the interleaved blocks
	else
	{
		for (int i = 1; i < scanner->n_blocks; i++)
			for (size_t j = 0; j < 8; j++)
				read_bit(scanner);
	}
	return res;
}
static unsigned int read_bits(scanner_t* scanner, size_t n)
{
	unsigned int res = 0;
	while (n)
	{
		if (!scanner->buf_avail)
		{
			scanner->buf = read_codeword(scanner);
			scanner->buf_avail = 8;
		}
		res = res*2 + ((scanner->buf >> (scanner->buf_avail-1))& 1);
		scanner->buf_avail--;
		n--;
	}
	return res;
}
void qrc_decode(bitmap_t* img)
{
	scanner_t _scanner;
	scanner_t* scanner = &_scanner;

	scanner->d = img->data;

	// size
	size_t s = img->width;
	if (s != img->height)
	{
		fprintf(stderr, "Non-square image\n");
		exit(1);
	}
	scanner->s = s;

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
	printf("Version %zu\n", v);
	scanner->v = v;

	// mask
	scanner->m = 4*P(s-3,8) + 2*P(s-4,8) + P(s-5,8);

	// error correction level
	int c = (!P(s-1,8))*2 + (!P(s-2,8)); // TODO
	if (c != 0)
	{
		fprintf(stderr, "Unsupported error correction level '%i'\n", c);
		exit(1);
	}

	static const int blocks[160][7] =
	{
#include "qrdata.h"
	};

	const int* b = blocks[4*(v-1) + c];
	scanner->blocks = b;
	// count all the blocks
	{
		int n = 0;
		for (size_t i = 0; b[i]; i+=3)
			n += b[i];
		scanner->n_blocks = n;
	}
	scanner->cur_block = 0;
	scanner->cur_words = b[2];
	scanner->cur_word = 0;

	// initialize reading
	scanner->i = s-1;
	scanner->j = s-1;
	scanner->buf = 0;
	scanner->buf_avail = 0;

	while (1)
	{
		byte enc = read_bits(scanner, 4);

		if (enc == 0) // END
		{
			break;
		}
		else if (enc == 1) // numeric
		{
			size_t lenbits = 10;
			if (v >= 27) lenbits = 14;
			else if (v >= 10) lenbits = 12;
			size_t len = read_bits(scanner, lenbits);

			for (; len>=3; len-=3)
			{
				unsigned int c = read_bits(scanner, 10);
				printf("%u", (c/100) % 10);
				printf("%u", (c/ 10) % 10);
				printf("%u", (c/  1) % 10);
			}
			if (len == 2)
			{
				unsigned int c = read_bits(scanner, 7);
				printf("%u", c/10);
				printf("%u", c%10);
			}
			else if (len == 1)
			{
				unsigned int c = read_bits(scanner, 4);
				printf("%u", c);
			}
		}
		else if (enc == 2) // alphanumeric
		{
			size_t lenbits = 9;
			if (v >= 27) lenbits = 13;
			else if (v >= 10) lenbits = 11;
			size_t len = read_bits(scanner, lenbits);

			static const char* map = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%%*+-./:";
			for (; len >= 2; len-=2)
			{
				unsigned int c = read_bits(scanner, 11);
				printf("%c", map[c/45]);
				printf("%c", map[c%45]);
			}
			if (len == 1)
			{
				unsigned int c = read_bits(scanner, 6);
				printf("%c", map[c]);
			}
		}
		else if (enc == 4) // Shift JIS
		{
			size_t lenbits = 8;
			if (v >= 10) lenbits = 16;
			size_t len = read_bits(scanner, lenbits);

			for (size_t i = 0; i < len; i++)
			{
				byte c = read_bits(scanner, 8);
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
