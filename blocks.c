#include "blocks.h"

#include <stdlib.h>
#include <stdio.h>

#include "modules.h"

static void get_block(scanner_t* scanner);

static void get_block(scanner_t* scanner)
{
	if (scanner->block_data == NULL)
	{
		scanner->block_data = (byte*) malloc(sizeof(byte));
	}
	scanner->block_data[0] = get_codeword(scanner);
	scanner->block_curbyte = 0;
	scanner->block_curbit = 0;
}

unsigned int get_bits(scanner_t* scanner, size_t n)
{
	unsigned int res = 0;
	while (n--)
	{
		if (scanner->block_curbyte >= 1) // TODO
			get_block(scanner);

		res *= 2;
		res += (scanner->block_data[scanner->block_curbyte] >> (7-scanner->block_curbit)) & 1;

		scanner->block_curbit++;
		if (scanner->block_curbit >= 8)
		{
			scanner->block_curbyte++;
			scanner->block_curbit = 0;
		}
	}
	return res;
}
