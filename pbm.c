#include "pbm.h"

#include <stdlib.h>
#include <string.h>

void pbm_name_to_bitmap(bitmap_t* img, const char* name)
{
	FILE* f = fopen(name, "rb");
	if (!f)
	{
		fprintf(stderr, "Could not open file '%s'\n", name);
		exit(1);
	}
	pbm_file_to_bitmap(img, f);
	fclose(f);
}

static byte nextint(FILE* f)
{
	char c;
	do
	{
		if (feof(f))
		{
			fprintf(stderr, "Unexpected end of file\n");
			exit(1);
		}
		c = fgetc(f);

		// pass comments
		if (c == '#')
			while (fgetc(f) != '\n'); // TODO: feof()

	} while (strchr(" \n\r", c));

	if (!('0' <= c && c <= '9'))
	{
		fprintf(stderr, "Unexpected character '%c'\n", c);
		exit(1);
	}

	byte res = 0;
	while ('0' <= c && c <= '9')
	{
		res *= 10;
		res += c - '0';
		if (feof(f))
			break;
		c = fgetc(f);
	}
	return res;
}
static byte nextbit(FILE* f, byte* buf, size_t* buf_avail)
{
	if (!*buf_avail)
	{
		fread(buf, 1, 1, f);
		*buf_avail = 8;
	}
	return (*buf >> (--(*buf_avail))) & 1;
}
void pbm_file_to_bitmap(bitmap_t* img, FILE* f)
{
	fgetc(f); // reads 'P'
	char format = fgetc(f); // reads format (1-6)

	byte w = nextint(f);
	byte h = nextint(f);

	color_t* d = (color_t*) malloc(sizeof(color_t) * w*h);
	if (!d)
	{
		fprintf(stderr, "Could not allocate memory\n");
		exit(1);
	}

	img->width  = w;
	img->height = h;
	img->data   = d;

	if (format == '1')
	{
		for (size_t i = 0; i < h; i++)
		for (size_t j = 0; j < w; j++)
		{
			byte v = nextint(f) ? 255 : 0;
			PIX(img, i, j).r = v;
			PIX(img, i, j).g = v;
			PIX(img, i, j).b = v;
		}
	}
	else if (format == '4')
	{
		byte buf = 0;
		size_t buf_avail = 0;
		for (size_t i = 0; i < h; i++)
		{
			for (size_t j = 0; j < w; j++)
			{
				byte v = nextbit(f, &buf, &buf_avail) ? 255 : 0;
				PIX(img, i, j).r = v;
				PIX(img, i, j).g = v;
				PIX(img, i, j).b = v;
			}
			buf_avail = 0;
		}
	}
	else
	{
		fprintf(stderr, "Unsupported format '%c'\n", format);
		exit(1);
	}
}

void pbm_del(bitmap_t* img)
{
	free(img->data);
}
