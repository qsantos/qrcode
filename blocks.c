#include "blocks.h"

#include <stdlib.h>
#include <stdio.h>

#include "modules.h"

static void get_block(scanner_t* scanner);

static void get_block(scanner_t* scanner)
{
	if (!scanner->block_dataw)
	{
		scanner->block_cur = 0;
		scanner->block_data = NULL;
	}
	else
		scanner->block_cur++;

	const byte* b = scanner->blocks;

	// find the size of the current block
	size_t ndata = scanner->block_cur < b[0] ? b[2] : b[5];
	scanner->block_dataw = ndata;

	// rewind
	scanner->i = scanner->s-1;
	scanner->j = scanner->s-1;

	// skip the previous blocks
	for (size_t i = 0; i < scanner->block_cur; i++)
		for (size_t j = 0; j < 8; j++)
			next_bit(scanner);

	free(scanner->block_data);
	scanner->block_data = (byte*) malloc(sizeof(byte) * ndata);

	scanner->cur_word = 0;
	for (size_t i = 0; i < ndata; i++)
		scanner->block_data[i] = get_codeword(scanner);

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
