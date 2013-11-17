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
#include "modules.h"
#include "bch.h"

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
static int  size_to_version(int ecl, size_t n);

static void set_finder   (scanner_t* scanner, size_t i, size_t j);
static void set_alignment(scanner_t* scanner, size_t i, size_t j);
static void set_format   (scanner_t* scanner, int ecl, byte mask);

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
		stream->d[stream->n] = 0;
		stream->b = 8;
	}
	stream->b--;
	stream->d[stream->n] |= bit << stream->b;
}

static void push_bits(stream_t* stream, size_t n, int v)
{
	if (!n) return;
	while (n)
	{
		n--;
		push_bit(stream, (v>>n) & 1);
	}
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
	push_bits(stream, 0, stream->b);
	stream->n++;
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

static void set_finder(scanner_t* scanner, size_t i, size_t j)
{
	for (size_t ki = 0; ki < 7; ki++)
	for (size_t kj = 0; kj < 7; kj++)
		P(i+ki, j+kj) = pattern_finder[ki][kj];
}

static void set_alignment(scanner_t* scanner, size_t i, size_t j)
{
	for (size_t ki = 0; ki < 5; ki++)
	for (size_t kj = 0; kj < 5; kj++)
		P(i+ki-2, j+kj-2) = pattern_alignment[ki][kj];
}

static void set_format(scanner_t* scanner, int ecl, byte mask)
{
	bch_t format = (ecl_to_code[ecl] << 3) | mask;
	format = bch_encode(bch_format_gen, format) ^ bch_format_mask;
	bch_t copy = format;

	// top-left
	for (int i = 0; i <= 5; i++)
	{
		P(i,8) = format & 1;
		format >>= 1;
	}
	P(7,8) = format & 1; format >>= 1;
	P(8,8) = format & 1; format >>= 1;
	P(8,7) = format & 1; format >>= 1;
	for (int i = 5; i >= 0; i--)
	{
		P(8,i) = format & 1;
		format >>= 1;
	}

	// bottom-left and top-right
	format = copy;
	size_t s = scanner->s;
	for (int i = 1; i <= 8; i++)
	{
		P(8, s-i) = format & 1;
		format >>= 1;
	}
	for (int i = 7; i >= 1; i--)
	{
		P(s-i, 8) = format & 1;
		format >>= 1;
	}
}

void qrc_encode(int ecl, const char* data)
{
	// generate bit stream
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
	fprintf(stderr, "Version %i selected\n", v);

	// create image
	size_t s = 17 + 4*v;
	byte* d = (byte*) calloc(s*s, sizeof(byte));
	if (d == NULL)
	{
		fprintf(stderr, "Could not allocate image\n");
		exit(1);
	}
	scanner_t _scanner = {d, s, v, ecl, 0, 0, 0, 0, 0, {0}, 0, 0, 0};
	scanner_t* scanner = &_scanner;

	// finder patterns
	set_finder(scanner, 0, 0);
	set_finder(scanner, s-7, 0);
	set_finder(scanner, 0, s-7);

	// timing patterns
	for (size_t i = 8; i < s-8; i += 2)
	{
		P(i, 6) = 1;
		P(6, i) = 1;
	}

	// alignment patterns
	const byte* xy = pattern_alignment_pos[v];
	for (const byte* a = xy; *a; a++)
		for (const byte* b = xy; *b; b++)
		{
			if (*a < 8 && *b < 8) continue; 
			if (*a < 8 && *b > s-9) continue;
			if (*b < 8 && *a > s-9) continue;
			set_alignment(scanner, *a, *b);
		}

	// version information
	if (v >= 7)
	{
		bch_t version = bch_encode(bch_version_gen, v) ^ bch_version_mask;
		for (int i = 0; i <= 5; i++)
			for (int j = 0; j <= 2; j++)
			{
				byte bit = version & 1;
				P(i, s-11+j) = bit;
				P(s-11+j, i) = bit;
				version >>= 1;
			}
	}

	// write data to image
	put_bits(scanner, stream.n, stream.d);

	// select a mask
	byte best_m = 0;
	int  best_s = 0;
	for (byte m = 0; m < 8; m++)
	{
		set_format(scanner, ecl, m);
		int s = mask_grade(scanner, m);
		if (s > best_s)
		{
			best_m = m;
			best_s = s;
		}
	}

	// apply the best mask
	mask_apply(scanner, best_m);

	// set format information
	set_format(scanner, ecl, best_m);

	printf("P1\n%zu %zu\n", s, s);
	for (size_t i = 0; i < s; i++)
	{
		for (size_t j = 0; j < s; j++)
		{
			if (d[i*s+j])
				printf("1 ");
			else
				printf("0 ");
		}
		printf("\n");
	}
}
