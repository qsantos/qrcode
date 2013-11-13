#include "modules.h"

static int  is_data (scanner_t* scanner, size_t i, size_t j);
static byte mask    (scanner_t* scanner, byte bit);
static void next_bit(scanner_t* scanner);
static void next_codeword(scanner_t* scanner);

static int is_data(scanner_t* scanner, size_t i, size_t j)
{
	size_t s = scanner->s;

	// finders and format information
	if (i <= 8   && j <= 8) return 0; // top-left
	if (i <= 8 && j >= s-8) return 0; // top-right
	if (j <= 8 && i >= s-8) return 0; // bottom-left

	if (scanner->v >= 7)
	{
		// version information
		if (i < 6 && j >= s-11) return 0; // top-right
		if (j < 6 && i >= s-11) return 0; // bottom-left
	}

	// timings
	if (i == 6) return 0;
	if (j == 6) return 0;

	// alignments
	if (i <= 8 && j >= s-10) return 1;
	if (j <= 8 && i >= s-10) return 1;
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

	return !(coll_x && coll_y);
}

static byte mask(scanner_t* scanner, byte bit)
{
	size_t i = scanner->i;
	size_t j = scanner->j;
	switch (scanner->m)
	{
	case 0: bit ^= 0 == (i*j)%2 + (i*j)%3;   break;
	case 1: bit ^= 0 == (i/2+j/3)%2;         break;
	case 2: bit ^= 0 == ((i*j)%3+(i+j)%2)%2; break;
	case 3: bit ^= 0 == ((i*j)%2+(i*j)%3)%2; break;
	case 4: bit ^= 0 == i%2;                 break;
	case 5: bit ^= 0 == (i+j)%2;             break;
	case 6: bit ^= 0 == (i+j)%3;             break;
	case 7: bit ^= 0 == j%3;                 break;
	default: return 0;
	}
	return bit;
}

static void next_bit(scanner_t* scanner)
{
	size_t i = scanner->i;
	size_t j = scanner->j;

	// next bit
	do
	{
		if ((j/2) % 2 == 0)
		{
			if (j % 2 == 0 || i >= scanner->s-1)
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
			if (j % 2 == 0 || i <= 0)
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
	while (!is_data(scanner,i,j));

	scanner->i = i;
	scanner->j = j;
}

static void next_codeword(scanner_t* scanner)
{
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
				next_bit(scanner);
	}
	// otherwise, skip the interleaved blocks
	else
	{
		for (int i = 1; i < scanner->n_blocks; i++)
			for (size_t j = 0; j < 8; j++)
				next_bit(scanner);
	}
}

byte get_codeword(scanner_t* scanner)
{
	byte res = 0;
	for (int i = 0; i < 8; i++)
	{
		byte bit = P(scanner->i, scanner->j);

		res *=2;
		res += mask(scanner, bit);
		next_bit(scanner);
	}

	next_codeword(scanner);
	return res;
}

void put_codeword(scanner_t* scanner, byte w)
{
	for (int i = 7; i >= 0; i--)
	{
		byte bit = (w>>i) & 1;
		next_bit(scanner);

		P(scanner->i, scanner->j) = mask(scanner, bit);
	}

	next_codeword(scanner);
}
