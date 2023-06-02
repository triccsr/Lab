#ifndef BITSET_H
#define BITSET_H
#include <stdint.h>
#include <stdbool.h>

int get_bitset_len(int bitNum);
void bitset_modify(uint64_t *bitset,int index,bool val);
bool bitset_get(uint64_t *bitset,int index);
#endif