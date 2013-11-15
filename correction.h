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

#ifndef CORRECTION_H
#define CORRECTION_H

#include "scanner.h"

typedef struct poly poly_t;

struct poly
{
	size_t d;    // degree
	byte c[512]; // coefficients
};

byte rs_calc_syndromes  (poly_t* msg, poly_t* synd);
void rs_forney_syndromes(poly_t* msg, poly_t* synd, poly_t* pos, poly_t* fsynd);
byte rs_find_error      (poly_t* msg, poly_t* synd, poly_t* pos);
void rs_correct_errata  (poly_t* msg, poly_t* synd, poly_t* pos);

byte rs_correction(size_t n_data, byte* data, byte nsym);

#endif
