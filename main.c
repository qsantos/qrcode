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

	bitmap_t img;
	pbm_file_to_bitmap(&img, f);

	qrc_decode(&img);

	pbm_del(&img);
	return 0;
}
