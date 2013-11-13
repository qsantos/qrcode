#ifndef QRMODULES_H
#define QRMODULES_H

#include "scanner.h"

void next_bit(scanner_t* scanner);
void next_codeword(scanner_t* scanner);

byte get_codeword(scanner_t* scanner);
void put_codeword(scanner_t* scanner, byte w);

#endif
