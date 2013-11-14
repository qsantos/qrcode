#include "blocks.h"

#include <stdlib.h>
#include <stdio.h>

#include "modules.h"

static void get_block(scanner_t* scanner);

static void get_block(scanner_t* scanner)
{
	// get block information
	static const byte blocks[160][7] =
	{
#include "blocksizes.h"
	};
	const byte* b = blocks[4*(scanner->v-1) + scanner->c];

	// current block
	size_t cur = scanner->block_cur;

	// find the data size of the current block
	size_t ndata = cur < b[0] ? b[2] : b[5];
	scanner->block_dataw = ndata;

	// rewind to start of symbol
	scanner->i = scanner->s-1;
	scanner->j = scanner->s-1;

	// skip the previous blocks' first codewords
	skip_bits(scanner, cur * 8);

	// read data
	// NOTE: the codewords are interleaved to skipping data from other blocks is
	//       necessary ; moreover, the second series of blocks can have one more
	//       codeword than the first, meaning that one more codeword is to be
	//       read and that the interleaving only affects the second kind of block
	size_t n = b[2] - 1; // n is either ndata-1 (first blocks) or ndata-2 (last ones)
	for (size_t i = 0; i < n; i++)
	{
		scanner->block_data[i] = get_codeword(scanner);

		// skip the interleaved codewords
		skip_bits(scanner, (b[0]+b[3]-1) * 8);
	}
	// last codeword common to both types of blocks
	scanner->block_data[n] = get_codeword(scanner);
	// additional codeword of the second type of blocks
	if (b[2] < ndata)
	{
		// skip the interleaved codewords
		skip_bits(scanner, (b[3]-1) * 8);

		scanner->block_data[n+1] = get_codeword(scanner);

	}
	else
	{
		// skip the last interleaved codewords
		skip_bits(scanner, b[3] * 8);
	}

	// number of error correction codewords
	// (same for both types of blocks)
	n = b[1] - b[2];
	for (size_t i = 0; i < n-1; i++)
	{
		scanner->block_data[ndata+i] = get_codeword(scanner);

		// skip the interleaved codewords
		skip_bits(scanner, (b[0]+b[3]-1) * 8);
	}

	scanner->block_cur = cur+1;
	scanner->block_curbyte = 0;
	scanner->block_curbit = 0;
}

unsigned int get_bits(scanner_t* scanner, size_t n)
{
	if (!scanner->block_cur)
		get_block(scanner);

	// this buffer handling is an abomination
	unsigned int res = 0;
	while (n--)
	{
		if (scanner->block_curbyte >= scanner->block_dataw)
			get_block(scanner);

		size_t B = scanner->block_curbyte;
		size_t b = scanner->block_curbit;
		res *= 2;
		res += (scanner->block_data[B] >> (7-b)) & 1;

		scanner->block_curbit++;
		if (scanner->block_curbit >= 8)
		{
			scanner->block_curbyte++;
			scanner->block_curbit = 0;
		}
	}
	return res;
}
