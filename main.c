#include <stdlib.h>

#include "pbm.h"
#include "qrcode.h"

int main(int argc, char** argv)
{
	FILE* f = argc > 1 ? fopen(argv[1], "r") : stdin;
	if (!f)
	{
		fprintf(stderr, "Could not open file\n");
		return 1;
	}

	scanner_t scanner;
	load_pbm(&scanner, f);

	qrc_decode(&scanner);

	free(scanner.d);
	return 0;
}
