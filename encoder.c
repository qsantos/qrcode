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
#include "blocks.h"

typedef struct stream stream_t;

struct stream
{
	int   a; // available memory (bytes)
	int   n; // used memory (bytes)
	char  b; // available bits in last byte
	char* d; // data

	size_t vr; // version range 0 = 1-9, 1 = 10-26, 2 = 27-40
};


static void push_bit    (stream_t* stream, char bit);
static void push_bits   (stream_t* stream, size_t n, int v);
static void push_segment(stream_t* stream, int enc, size_t n, const char* str);
static void encode_in_range(stream_t* stream, const char* data);

static void push_bit(stream_t* stream, char bit)
{
	if (stream->b == 0)
	{
		stream->n++;
		if (stream->n >= stream->a)
		{
			stream->a = stream->a ? 2*stream->a : 1;
			stream->d = realloc(stream->d, stream->a);
			if (stream->d == NULL)
			{
				fprintf(stderr, "Could not allocate memory for encoding stream\n");
				exit(1);
			}
		}
		stream->b = 8;
	}
	stream->d[stream->n] |= bit << (stream->b-1);
	stream->b--;
}

static void push_bits(stream_t* stream, size_t n, int v)
{
	printf("Pushing %#x (%zu)\n", v, n);
	if (!n) return;
	while (--n)
		push_bit(stream, v>>n);
}

#define D(I) ((str[I]) - '0')
#define A(I) (strchr(charset_alpha, str[I]) - charset_alpha)
static void push_segment(stream_t* stream, int enc, size_t n, const char* str)
{
	if (!n) return;

	push_bits(stream, 4, enc); // mode
	if (enc)
		push_bits(stream, lenbits[enc][stream->vr], n); // length
	if (enc == 1)
	{
		for (; n>=3; n-=3, str+=3)
		{
			unsigned int c = (D(0)*10 + D(1))*10 + D(2);
			push_bits(stream, 10, c);
		}
		if (n == 2)
		{
			unsigned int c = D(0)*10 + D(1);
			push_bits(stream, 7, c);
		}
		else if (n == 1)
		{
			unsigned int c = D(0);
			push_bits(stream, 4, c);
		}
	}
	else if (enc == 2)
	{
		for (; n>=2; n-=2, str+=2)
		{
			unsigned int c = A(0)*45 + A(1);
			push_bits(stream, 11, c);
		}
		if (n == 1)
		{
			unsigned int c = A(0);
			push_bits(stream, 5, c);
		}
	}
	else if (enc == 4)
	{
		for (; n; n--, str++)
			push_bits(stream, 8, *str);
	}
	else
	{
		exit(1);
	}
}

#define PUSH(E,N) {push_segment(stream,E,N,data); data += (N);}
static void encode_in_range(stream_t* stream, const char* data)
{
	stream->n = -1;
	stream->b = 0;

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
	push_bits(stream, 4, 0);
}

static int size_to_version(int ecl, size_t n)
{
	for (size_t v = 1; v <= 40; v++)
	{
		const byte* b = block_sizes[4*v+ecl];
		size_t c = b[0]*b[2] + b[3]*b[5];
		if (n <= c)
			return v;
	}
	return -1;
}

void qrc_encode(int ecl, const char* data)
{
	stream_t stream;
	stream.a = 0;
	stream.d = NULL;
	stream.vr = 0;
	int v;
	while (1)
	{
		encode_in_range(&stream, data);
		v = size_to_version(ecl, stream.n);
		if (v < 0)
		{
			fprintf(stderr, "The data cannot fit in any QR-code\n");
			exit(1);
		}

		size_t vr = version_range[v];
		if (vr == stream.vr)
			break;
		stream.vr = vr;
	}
	printf("Version %i selected\n", v);

	size_t s = 17 + 4*v;
	byte* d = (byte*) malloc(s*s);
	if (d == NULL)
	{
		fprintf(stderr, "Could not allocate image\n");
		exit(1);
	}

	scanner_t scanner = {d, s, v, ecl, 0, 0, 0, 0, 0, {0}, 0, 0, 0};
	put_bits(&scanner, stream.n, stream.d);
}
