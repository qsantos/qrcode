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
#include "encoder.h"
#include "decoder.h"



// please, refer to DOCUMENTATION for technical details



static char* readfile(FILE* f)
{
	size_t n = 0;
	size_t a = 1;
	char* ret = malloc(1);
	while (fread(ret+n, 1, a-n, f) == a-n)
	{
		n = a;
		a *= 2;
		ret = realloc(ret, a);
	}
	return ret;
}
static void usage(char* name)
{
	fprintf(stderr,
		"Usage: %s [option] action [data]\n"
		"QR-Code decoder\n"
		"\n"
		"  -V, --version       display the version of this program\n"
		"  -h, --help          print this help\n"
		"\n"
		"  -e, --encode        encode to QR-code (data is a string)\n"
		"  -f, --encfile       encode to QR-code (data is a file)\n"
		"  -d, --decode        decode from QR-code (data is a file)\n"
		"  -n, --use-netpbm    use the netpbm representation instead of ascii characters\n"
		"\n"
		"  -v  --verbose  increase verbosity\n"
		, name);
}
int main(int argc, char** argv)
{
	scanner_t scanner;
	scanner.c = 0;
	scanner.verbosity = 0;
	scanner.use_netpbm = 0;

	int curarg = 1;
	int action = 0; // 1 = encode string, 2 = encode file, 3 = decode file
	const char* data = NULL;
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
		else if (strcmp(arg, "--use-netpbm") == 0 || strcmp(arg, "-n") == 0)
		{
			scanner.use_netpbm = 1;
		}
		else if (strcmp(arg, "--encode") == 0 || strcmp(arg, "-e") == 0)
		{
			action = 1;
		}
		else if (strcmp(arg, "--encfile") == 0 || strcmp(arg, "-f") == 0)
		{
			action = 2;
		}
		else if (strcmp(arg, "--decode") == 0 || strcmp(arg, "-d") == 0)
		{
			action = 3;
		}
		else
		{
			data = arg;
			break;
		}
	}

	if (action == 0)
	{
		fprintf(stderr, "No action specified\n");
		exit(1);
	}
	else if (action == 1) // encode
	{
		qrc_encode(&scanner, data);
	}
	else if (action == 2) // encfile
	{
		FILE* f = data ? fopen(data, "r") : stdin;
		if (!f)
		{
			fprintf(stderr, "Could not read file '%s'\n", data);
			exit(1);
		}
		char* str = readfile(f);
		qrc_encode(&scanner, str);
		free(str);
	}
	else if (action == 3) // decode
	{
		FILE* f = data ? fopen(data, "r") : stdin;
		if (!f)
		{
			fprintf(stderr, "Could not open file '%s'\n", data);
			exit(1);
		}
		load_pbm(&scanner, f);
		qrc_decode(&scanner);
	}

	free(scanner.d);
	return 0;
}
