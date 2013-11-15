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

#include "bch.h"

static int leading_bit(unsigned int x);
static int hamming_weight(bch_t code);

static int leading_bit(unsigned int x)
{
	return 31 - __builtin_clz(x);
}

bch_t bch_check(bch_t g, bch_t code)
{
	int ncode = leading_bit(code);
	int ngen  = leading_bit(g);
	bch_t mask = 1 << ncode;
	for (int i = ncode-ngen; i >= 0; i--, mask >>= 1)
		if (code & mask)
			code ^= g << i;
	return code;
}

bch_t bch_encode(bch_t g, bch_t code)
{
	int ngen  = leading_bit(g);
	code <<= ngen;
	return code ^ bch_check(g, code);
}

static int hamming_weight(bch_t code)
{
	return __builtin_popcount(code);
}

bch_t bch_decode(bch_t g, bch_t code)
{
	int n_bits = leading_bit(code) / 2;
	int maxn = 1<<n_bits;

	bch_t best_v = -1;
	int   best_d = 64;
	for (int v = 0; v < maxn; v++)
	{
		bch_t test = bch_encode(g, v);
		int d = hamming_weight(code ^ test);
		if (d < best_d)
		{
			best_v = v;
			best_d = d;
		}
		else if (d == best_d) // conflict
		{
			best_v = -1;
		}
	}
	return best_v;
}
