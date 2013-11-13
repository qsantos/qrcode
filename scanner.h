#ifndef QRSCANNER_H
#define QRSCANNER_H

#include <sys/types.h>
typedef unsigned char byte;
typedef struct scanner scanner_t;

#define P(I,J) (scanner->d[(I)*scanner->s + (J)])

struct scanner
{
	size_t s; // size
	int    v; // version
	byte*  d; // data
	int    m; // mask

	// current module
	size_t i;
	size_t j;

	// block splitting
	const byte* blocks; // block information
	size_t block_count;   // total number of blocks
	size_t block_cur;     // current block
	size_t block_dataw;   // number of its data words
	size_t cur_word;      // current word

	// block buffer
	byte*  block_data;
	size_t block_curbyte;
	size_t block_curbit;
};

#endif
