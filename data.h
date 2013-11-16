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

#ifndef DATA_H
#define DATA_H

extern const unsigned char pattern_finder[7][7];

extern const char* charset_alpha;

extern const unsigned char version_range[41];
extern const unsigned char lenbits[5][3]; // [encoding][version range]

extern const unsigned long bch_format_gen;
extern const unsigned long bch_format_mask;
extern const unsigned long bch_version_gen;
extern const unsigned long bch_version_mask;

extern const unsigned char block_sizes[160][7];

#endif
