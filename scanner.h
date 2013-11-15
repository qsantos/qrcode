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

#ifndef QRSCANNER_H
#define QRSCANNER_H

#include <sys/types.h>

typedef unsigned char byte;
typedef struct scanner scanner_t;

#define P(I,J) (scanner->d[(I)*scanner->s + (J)])

struct scanner
{
	byte*  d; // data
	size_t s; // size
	int    v; // version
	int    c; // correction level
	int    m; // mask

	// current module
	size_t i;
	size_t j;

	// block splitting
	size_t block_cur;     // current block
	size_t block_dataw;   // number of its data words

	// block buffer
	byte   block_data[163]; // max block length is 163
	size_t block_curbyte;
	size_t block_curbit;

	// options
	char verbosity; // verbosity level
};

#endif
