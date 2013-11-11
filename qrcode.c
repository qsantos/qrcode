#include "qrcode.h"

#include <stdlib.h>

void qrc_decode(bitmap_t* img)
{
	if (img->width != img->height)
	{
		fprintf(stderr, "Non-square image\n");
		exit(1);
	}

	size_t v = img->width - 17;
	if (v % 4 != 0)
	{
		fprintf(stderr, "Invalid image size\n");
		exit(1);
	}
	v /= 4;
	
	if (v != 4)
	{
		fprintf(stderr, "Unsupported version '%zu'\n", v);
		exit(1);
	}
}
