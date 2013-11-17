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

#ifndef QRMODULES_H
#define QRMODULES_H

#include "scanner.h"

void next_bit (scanner_t* scanner);
void skip_bits(scanner_t* scanner, size_t n);

byte mask      (byte m, size_t i, size_t j);
int  mask_grade(scanner_t* scanner, byte m);
void mask_apply(scanner_t* scanner, byte m);

byte get_codeword(scanner_t* scanner);
void put_codeword(scanner_t* scanner, byte w);

#endif
