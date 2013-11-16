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

#include "data.h"

typedef struct encoded encoded_t;

struct encoded
{
	int   a; // available memory (bytes)
	int   n; // used memory (bytes)
	char  b; // available bits in last byte
	char* d; // data

	int vr; // version range 0 = 1-9, 1 = 10-26, 2 = 27-40
};


static void push_bit    (encoded_t* encoded, char bit);
static void push_bits   (encoded_t* encoded, size_t n, int v);
static void push_segment(encoded_t* encoded, int enc, size_t n, const char* str);
static void encode_in_range(encoded_t* encoded, const char* data);

static void push_bit(encoded_t* encoded, char bit)
{
	if (encoded->b == 0)
	{
		encoded->n++;
		if (encoded->n >= encoded->a)
		{
			encoded->a = encoded->a ? 2*encoded->a : 1;
			encoded->d = realloc(encoded->d, encoded->a);
		}
		encoded->b = 8;
	}
	encoded->d[encoded->n] |= bit << (encoded->b-1);
	encoded->b--;
}

static void push_bits(encoded_t* encoded, size_t n, int v)
{
	printf("Pushing %#x (%zu)\n", v, n);
	if (!n) return;
	while (--n)
		push_bit(encoded, v>>n);
}

#define D(I) ((str[I]) - '0')
#define A(I) (strchr(charset_alpha, str[I]) - charset_alpha)
static void push_segment(encoded_t* encoded, int enc, size_t n, const char* str)
{
	if (!n) return;

	push_bits(encoded, 4, enc); // mode
	if (enc)
		push_bits(encoded, lenbits[enc][encoded->vr], n); // length
	if (enc == 1)
	{
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
		for (; n; n--, str++)
			push_bits(encoded, 8, *str);
	}
	else
	{
		exit(1);
	}
}

#define PUSH(E,N) {push_segment(encoded,E,N,data); data += (N);}
static void encode_in_range(encoded_t* encoded, const char* data)
{
	encoded->n = -1;
	encoded->b = 0;

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
	push_bits(encoded, 4, 0);
}

void qrc_encode(const char* data)
{
	encoded_t encoded;
	encoded.a = 0;
	encoded.d = NULL;
	encoded.vr = 0;
	encode_in_range(&encoded, data);
}
