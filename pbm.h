#ifndef PBM_H
#define PBM_H

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
	byte*  data;
};

#endif
