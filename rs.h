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

#ifndef RS_H
#define RS_H

#include "scanner.h"

typedef struct poly poly_t;

// NOTE: for 'synd' and 'pos', the poly_t structure is used as an array and
//       the 'd' parameter is actually the size of this array (the degree of
//       a polynomial is the number of coefficients minus one)
struct poly
{
	size_t d;    // degree
	byte c[512]; // coefficients
};

void rs_encode(size_t n_data, byte* data, byte n_sym);
byte rs_decode(size_t n_data, byte* data, byte n_sym);

#endif
