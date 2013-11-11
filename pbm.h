#ifndef PBM_H
#define PBM_H

#include <stdio.h>

typedef unsigned char byte;

typedef struct color  color_t;
typedef struct bitmap bitmap_t;

struct color
{
	byte r;
	byte g;
	byte b;
};

struct bitmap
{
	size_t width;
	size_t height;
	color_t* data;
};

#define PIX(S,I,J) ((S)->data[(I)*(S)->width + (J)])
void pbm_name_to_bitmap(bitmap_t* img, const char* name);
void pbm_file_to_bitmap(bitmap_t* img, FILE* f);

void pbm_del(bitmap_t* img);

#endif
