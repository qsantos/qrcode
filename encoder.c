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

#include "encoder.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct encoded encoded_t;

struct encoded
{
	size_t a; // available memory (bytes)
	size_t n; // used memory (bytes)
	char   b; // available bits in last byte
	char*  d; // data
};

static const char* charset_alpha = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";

static void push_bit(encoded_t* encoded, char bit)
{
	if (encoded->b == 0)
	{
		if (encoded->n >= encoded->a)
		{
			encoded->a = encoded->a ? 2*encoded->a : 1;
			encoded->d = realloc(encoded->d, encoded->a);
		}
		encoded->n++;
		encoded->b = 8;
	}
	encoded->d[encoded->n] |= bit << (encoded->b-1);
	encoded->b--;
}
static void push_bits(encoded_t* encoded, size_t n, int v)
{
	printf("Pushing %#x (%zu)\n", v, n);
	while (--n)
		push_bit(encoded, v>>n);
}
#define D(I) ((str[I]) - '0')
#define A(I) (strchr(charset_alpha, str[I]) - charset_alpha)
static void push_segment(encoded_t* encoded, int enc, size_t n, const char* str)
{
	if (!n)
		return;

	push_bits(encoded, 4, enc);
	if (enc == 1)
	{
		push_bits(encoded, 10, n);
		for (; n>=3; n-=3, str+=3)
		{
			unsigned int c = (D(0)*10 + D(1))*10 + D(2);
			push_bits(encoded, 10, c);
		}
		if (n == 2)
		{
			unsigned int c = D(0)*10 + D(1);
			push_bits(encoded, 7, c);
		}
		else if (n == 1)
		{
			unsigned int c = D(0);
			push_bits(encoded, 4, c);
		}
	}
	else if (enc == 2)
	{
		push_bits(encoded, 9, n);
		for (; n>=2; n-=2, str+=2)
		{
			unsigned int c = A(0)*45 + A(1);
			push_bits(encoded, 11, c);
		}
		if (n == 1)
		{
			unsigned int c = A(0);
			push_bits(encoded, 5, c);
		}
	}
	else if (enc == 4)
	{
		push_bits(encoded, 8, n);
		for (; n; n--, str++)
			push_bits(encoded, 8, *str);
	}
	else
	{
		exit(1);
	}
}
#define PUSH(E,N) {push_segment(&encoded,E,N,data); data += (N);}
void qrc_encode(const char* data)
{
	encoded_t encoded = { 0, 0, 0, NULL };

	size_t n_numer = 0;
	size_t n_alpha = 0;
	size_t n_byte  = 0;
	for (const char* c = data; *c; c++)
	{
		if ('0' <= *c && *c <= '9')
		{
			n_numer++;
			n_alpha++;
		}
		else
		{
			if (n_numer >= 6)
			{
				if (n_alpha - n_numer >= 11)
				{
					PUSH(4, n_byte-n_alpha)
					PUSH(2, n_alpha-n_numer)
				}
				else
				{
					PUSH(4, n_byte-n_alpha)
				}
				PUSH(1, n_numer)
				n_alpha = 0;
				n_byte = 0;
			}

			if (strchr(charset_alpha+10, *c)) // skip digits
			{
				n_alpha++;
			}
			else
			{
				if (n_alpha >= 11)
				{
					PUSH(4, n_byte-n_alpha)
					PUSH(2, n_alpha)
					n_byte = 0;
				}
				n_alpha = 0;
			}
			n_numer = 0;
		}
		n_byte++;
	}
	PUSH(4, n_byte)
	push_bits(&encoded, 4, 0);
}
