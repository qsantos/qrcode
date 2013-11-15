/*\
 *  Yes, This Is Another Barcode Reader
 *  Copyright (C) 2013 Quentin SANTOS
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
\*/

#include "blocks.h"

#include <stdlib.h>
#include <stdio.h>

#include "modules.h"
#include "correction.h"

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
	size_t n_data = cur < b[0] ? b[2] : b[5];
	scanner->block_dataw = n_data;

	// rewind to start of symbol
	scanner->i = scanner->s-1;
	scanner->j = scanner->s-1;


	// NOTE: in a symbol, all the blocks have the same number of error
	//       correction codewords but the first series of blocks can
	//       have one data codeword less

	byte nblocks = b[0]+ b[3];



	// BEGIN read data
	// the next section handles the inverleaving of data codewords

	// handling the minimal number of codewords
	// n is either n_data-1 (first blocks) or n_data-2 (last ones)
	for (size_t i = 0; i < b[2]; i++)
	{
		skip_bits(scanner, cur * 8);
		scanner->block_data[i] = get_codeword(scanner);
		skip_bits(scanner, (nblocks-cur-1) * 8);
	}

	// interleaving specific to the second series of blocks
	if (b[2] == n_data) // first kind of block
	{
		skip_bits(scanner, b[3] * 8);
	}
	else // second kind
	{
		skip_bits(scanner, (cur-b[0]) * 8);
		scanner->block_data[n_data-1] = get_codeword(scanner);
		skip_bits(scanner, (nblocks-cur-1) * 8);

	}

	// END read data



	// the module pointers is now at the end of all the data and at
	// the beginning of the interleaved error correction codewords



	// BEGIN read correction
	// this section handles the interleaving of error correction codewords

	size_t n_errwords = b[1] - b[2]; // same for both types of blocks
	for (size_t i = 0; i < n_errwords; i++)
	{
		skip_bits(scanner, cur * 8);
		scanner->block_data[n_data+i] = get_codeword(scanner);
		skip_bits(scanner, (nblocks-cur-1) * 8);
	}

	// END read correction



	// apply Reed-Solomon error correction
	if (rs_correction(n_data+n_errwords, scanner->block_data, n_errwords) != 0)
	{
		fprintf(stderr, "Could not correct errors\n");
		exit(1);
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
