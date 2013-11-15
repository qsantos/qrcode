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
	int    c; // correction level
	byte*  d; // data
	int    m; // mask

	// current module
	size_t i;
	size_t j;

	// block splitting
	size_t block_cur;     // current block
	size_t block_dataw;   // number of its data words

	// block buffer
	byte   block_data[163]; // max block length is 163
	size_t block_curbyte;
	size_t block_curbit;

	// options
	char verbosity; // verbosity level
};

#endif
