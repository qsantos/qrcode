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
		, name);
}
int main(int argc, char** argv)
{
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

	scanner_t scanner;
	load_pbm(&scanner, f);

	qrc_decode(&scanner);

	free(scanner.d);
	return 0;
}
