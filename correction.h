#ifndef CORRECTION_H
#define CORRECTION_H

#include "scanner.h"

typedef struct poly poly_t;

struct poly
{
	size_t d;    // degree
	byte c[512]; // coefficients
};

byte rs_calc_syndromes  (poly_t* msg, poly_t* synd);
byte rs_find_error      (poly_t* msg, poly_t* synd, poly_t* pos);
void rs_correct_errata  (poly_t* msg, poly_t* synd, poly_t* pos);

#endif
