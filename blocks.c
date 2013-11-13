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
	size_t cur = scanner->block_dataw ? scanner->block_cur+1 : 0;

	// find the data size of the current block
	size_t ndata = cur < b[0] ? b[2] : b[5];
	scanner->block_dataw = ndata;

	// rewind to start of symbol
	scanner->i = scanner->s-1;
	scanner->j = scanner->s-1;

	// skip the previous blocks' first codewords
	skip_bits(scanner, cur * 8);

	// read data
	for (size_t i = 0; i < ndata; i++)
	{
		scanner->block_data[i] = get_codeword(scanner);

		// skip the interleaved codewords
		skip_bits(scanner, (b[0]+b[3]-1) * 8);
	}

	scanner->block_cur = cur;
	scanner->block_curbyte = 0;
	scanner->block_curbit = 0;
}

unsigned int get_bits(scanner_t* scanner, size_t n)
{
	// this buffer handling is an abomination
	unsigned int res = 0;
	while (n--)
	{
		// this condition is an abomination
		if (!scanner->block_dataw || scanner->block_curbyte >= scanner->block_dataw)
			get_block(scanner);

		// this bit-field handling is an abomination
		res *= 2;
		res += (scanner->block_data[scanner->block_curbyte] >> (7-scanner->block_curbit)) & 1;

		// meh
		scanner->block_curbit++;
		if (scanner->block_curbit >= 8)
		{
			scanner->block_curbyte++;
			scanner->block_curbit = 0;
		}
	}
	return res;
}
