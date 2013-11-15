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

#include "decoder.h"

#include <stdlib.h>

#include "blocks.h"
#include "bch.h"



// please, refer to DOCUMENTATION for technical details



static void check_finder(scanner_t* scanner, size_t i, size_t j);

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
	if (v >= 7)
	{
		int version1 = 0;
		for (size_t i = 0; i < 6; i++)
			for (size_t j = 0; j < 3; j++)
				version1 = 2*version1 + P(i, s-11+j);
		version1 = bch_decode(0x1f25, version1);

		int version2 = 0;
		for (size_t i = 0; i < 6; i++)
			for (size_t j = 0; j < 3; j++)
				version2 = 2*version2 + P(i, s-11+j);
		version2 = bch_decode(0x1f25, version2);

		if ((version1 < 0 && version2 < 0) || version1 != version2)
		{
			fprintf(stderr, "Version information corrupted\n");
			exit(1);
		}

		if (version1 != (int) v)
		{
			fprintf(stderr, "Version information mismatches size\n");
			exit(1);
		}
	}

	// format information
	bch_t format1 = 0;
	for (int i = 0; i <= 5; i++)
		format1 = 2*format1 + P(8,i);
	format1 = 2*format1 + P(8,7);
	format1 = 2*format1 + P(8,8);
	format1 = 2*format1 + P(7,8);
	for (int i = 5; i >= 0; i--)
		format1 = 2*format1 + P(i,8);
	format1 = bch_decode(0x537, format1 ^ 0x5412);

	bch_t format2 = 0;
	for (int i = 1; i <= 7; i++)
		format2 = 2*format2 + P(s-i,8);
	for (int i = 8; i >= 1; i--)
		format2 = 2*format2 + P(8,s-i);
	format2 = bch_decode(0x537, format2 ^ 0x5412);

	if ((format1 < 0 && format2 < 0) || format1 != format2)
	{
		fprintf(stderr, "Format information corrupted\n");
		exit(1);
	}

	int format = format1 & format2;
	scanner->m = format & 0x7;      // mask
	byte ECLs[] = { 1, 0, 3, 2 };   // yes, absurd (see Table 25)
	scanner->c = ECLs[format >> 3]; // Error Correction Level

	if (scanner->verbosity > 0)
		printf("Version %zu-%c (mask %u)\n", v, "LMQH"[scanner->c], scanner->m);

	// decode data
	scanner->block_cur = 0;
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

			static const char* map = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";
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