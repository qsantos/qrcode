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

#include "modules.h"



// please, refer to DOCUMENTATION for technical details



static int  is_data (scanner_t* scanner, size_t i, size_t j);
static byte mask    (scanner_t* scanner, byte bit);

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
	case 0: bit ^= 0 == (i+j)%2;             break;
	case 1: bit ^= 0 == i%2;                 break;
	case 2: bit ^= 0 == j%3;                 break;
	case 3: bit ^= 0 == (i+j)%3;             break;
	case 4: bit ^= 0 == (i/2+j/3)%2;         break;
	case 5: bit ^= 0 == (i*j)%2 + (i*j)%3;   break;
	case 6: bit ^= 0 == ((i*j)%2+(i*j)%3)%2; break;
	case 7: bit ^= 0 == ((i*j)%3+(i+j)%2)%2; break;
	default: return bit;
	}
	return bit;
}

void next_bit(scanner_t* scanner)
{
	size_t i = scanner->i;
	size_t j = scanner->j;

	// next bit
	do
	{
		if ((j/2) % 2 == 0)
		{
			if (j % 2 == (j<6) || i >= scanner->s-1)
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
			if (j % 2 == (j<6) || i <= 0)
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

void skip_bits(scanner_t* scanner, size_t n)
{
	while (n--)
		next_bit(scanner);
}

byte get_codeword(scanner_t* scanner)
{
	byte res = 0;
	for (int i = 0; i < 8; i++)
	{
		byte bit = P(scanner->i, scanner->j);

		res *= 2;
		res += mask(scanner, bit);
		next_bit(scanner);
	}

	return res;
}

void put_codeword(scanner_t* scanner, byte w)
{
	for (int i = 7; i >= 0; i--)
	{
		byte bit = (w>>i) & 1;
		next_bit(scanner);

		P(scanner->i, scanner->j) = bit;
	}
}
