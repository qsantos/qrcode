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

#include <stdlib.h>
#include <string.h>

#include "pbm.h"
#include "qrcode.h"

static void usage(char* name)
{
	fprintf(stderr,
		"Usage: %s [option] [file]\n"
		"QR-Code decoder\n"
		"\n"
		"  -V, --version  display the version of this program\n"
		"  -h, --help     print this help\n"
		"\n"
		"  -v  --verbose  increase verbosity\n"
		, name);
}
int main(int argc, char** argv)
{
	scanner_t scanner;
	scanner.verbosity = 0;

	FILE* f = stdin;

	int curarg = 1;
	while (curarg < argc)
	{
		const char* arg = argv[curarg++];

		if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)
		{
			usage(argv[0]);
			exit(1);
		}
		else if (strcmp(arg, "--version") == 0 || strcmp(arg, "-V") == 0)
		{
			fprintf(stderr, "qrdecoder version 0.1\n");
			fprintf(stderr, "Compiled on %s at %s\n", __DATE__, __TIME__);
			exit(1);
		}
		else if (strcmp(arg, "--verbose") == 0 || strcmp(arg, "-v") == 0)
		{
			scanner.verbosity = 1;
		}
		else
		{
			f = fopen(arg, "r");
			if (!f)
			{
				fprintf(stderr, "Could not open file '%s'\n", arg);
				return 1;
			}
		}
	}

	load_pbm(&scanner, f);
	qrc_decode(&scanner);
	free(scanner.d);
	return 0;
}
