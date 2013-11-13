#ifndef QRMODULES_H
#define QRMODULES_H

#include "qrscanner.h"

byte get_codeword(scanner_t* scanner);
void put_codeword(scanner_t* scanner, byte w);

#endif
