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

#include "data.h"



// please, refer to DOCUMENTATION for technical details



static int is_data(scanner_t* scanner, byte i, byte j);

static int is_data(scanner_t* scanner, byte i, byte j)
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
	int coll_x = 0;
	for (const byte* a = pattern_alignment_pos[scanner->v]; *a && !coll_x; a++)
		coll_x = (*a-2 <= i && i <= *a+2);
	int coll_y = 0;
	for (const byte* a = pattern_alignment_pos[scanner->v]; *a && !coll_y; a++)
		coll_y = (*a-2 <= j && j <= *a+2);

	return !(coll_x && coll_y);
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
			if (j % 2 == (j<6))
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

byte mask(byte m, size_t i, size_t j)
{
	switch (m)
	{
	case 0: return 0 == (i+j)%2;
	case 1: return 0 == i%2;
	case 2: return 0 == j%3;
	case 3: return 0 == (i+j)%3;
	case 4: return 0 == (i/2+j/3)%2;
	case 5: return 0 == (i*j)%2 + (i*j)%3;
	case 6: return 0 == ((i*j)%2+(i*j)%3)%2;
	case 7: return 0 == ((i*j)%3+(i+j)%2)%2;
	default: return 0;
	}
}

byte mask_if_content(scanner_t* scanner, byte m, size_t i, size_t j)
{
	byte bit = P(i,j);
	if (is_data(scanner, i, j))
		bit ^= mask(m, i, j);
	return bit;
}
#define B(i,j) mask_if_content(scanner, m, i, j)
int mask_grade(scanner_t* scanner, byte m)
{
	// ugly, but fast enough

	size_t s = scanner->s;
	int score = 0;

	// N_1
	// rows
	for (size_t i = 0; i < s; i++)
	{
		byte cur_color = 0;
		size_t n_cons = 0;
		for (size_t j = 0; j < s; j++)
		{
			byte bit = B(i,j);

			if (bit != cur_color)
			{
				n_cons = 0;
				cur_color = bit;
			}
			else if (n_cons == 5)
				score += 3;
			else if (n_cons > 5)
				score++;
			n_cons++;
		}
	}
	// columns
	for (size_t j = 0; j < s; j++)
	{
		byte cur_color = 0;
		size_t n_cons = 0;
		for (size_t i = 0; i < s; i++)
		{
			byte bit = B(i,j);

			if (bit != cur_color)
			{
				n_cons = 0;
				cur_color = bit;
			}
			else if (n_cons == 5)
				score += 3;
			else if (n_cons > 5)
			n_cons++;
		}
	}

	// N_2
	// just adding 3 for each 2x2 block (with overlap)
	for (size_t i = 0; i < s-1; i++)
		for (size_t j = 0; j < s-1; j++)
		{
			byte bit00 = B(i+0,j+0);
			byte bit01 = B(i+0,j+1);
			byte bit10 = B(i+1,j+0);
			byte bit11 = B(i+1,j+1);
			if (bit00 == bit01 && bit10 == bit11 && bit00 == bit10)
				score += 3;
		}

	// N_3
	// considering what we have done, quadratic time is not that bad
	for (size_t i = 0; i < s; i++)
		for (size_t j = 0; j < s-6; j++)
		{
			byte bit0 = B(i, j+0);
			byte bit1 = B(i, j+1);
			byte bit2 = B(i, j+2);
			byte bit3 = B(i, j+3);
			byte bit4 = B(i, j+4);
			byte bit5 = B(i, j+5);
			byte bit6 = B(i, j+6);
			if (bit0 && !bit1 && bit2 && bit3 && bit4 && !bit5 && bit6)
				score += 40;
		}
	for (size_t j = 0; j < s; j++)
		for (size_t i = 0; i < s-6; i++)
		{
			byte bit0 = B(i+0, j);
			byte bit1 = B(i+1, j);
			byte bit2 = B(i+2, j);
			byte bit3 = B(i+3, j);
			byte bit4 = B(i+4, j);
			byte bit5 = B(i+5, j);
			byte bit6 = B(i+6, j);
			if (bit0 && !bit1 && bit2 && bit3 && bit4 && !bit5 && bit6)
				score += 40;
		}

	// N_4
	// ratio is an integer value between 0 and 20
	// the middle is 10 and each unit maps to 5%
	size_t n_dark = 0;
	for (size_t i = 0; i < s; i++)
		for (size_t j = 0; j < s; j++)
			if (B(i,j))
				n_dark++;
	int ratio = (20 * n_dark) / (s*s);
	int k = ratio - 10;
	if (k < 0)
		k = -k;
	score += 10 * k;

	return score;
}

void mask_apply(scanner_t* scanner, byte m)
{
	size_t s = scanner->s;
	for (size_t i = 0; i < s; i++)
		for (size_t j = 0; j < s; j++)
			if (is_data(scanner, i, j))
				P(i,j) ^= mask(m, i, j);
}

byte get_codeword(scanner_t* scanner)
{
	byte res = 0;
	byte m = scanner->m;
	for (int i = 0; i < 8; i++)
	{
		size_t i = scanner->i;
		size_t j = scanner->j;

		res *= 2;
		res += P(i,j) ^ mask(m, i, j);
		next_bit(scanner);
	}

	return res;
}

void put_codeword(scanner_t* scanner, byte w)
{
	for (int i = 7; i >= 0; i--)
	{
		byte bit = (w>>i) & 1;

		P(scanner->i, scanner->j) = bit;
		next_bit(scanner);
	}
}
