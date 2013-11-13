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

	// block splitting
	const int* blocks; // block information
	int block_count;   // total number of blocks
	int block_cur;     // current block
	int block_dataw;   // number of its data words
	int cur_word;      // current word

	// current bit
	size_t i;
	size_t j;

	// codeword buffer
	byte   buf;
	size_t buf_avail;
};

#endif
