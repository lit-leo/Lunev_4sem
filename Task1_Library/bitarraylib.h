#ifndef _BITARRAYLIB_H_
#define _BITARRAYLIB_H_

typedef struct bitArray
{
    unsigned int* array;
    unsigned int capacity;
} bitArray_t;

int bitArrayCtor(bitArray_t *this, unsigned int range);

int bitArrayDtor(bitArray_t *this);

int bitArraySet(bitArray_t *this, unsigned int index);

int bitArrayTest(bitArray_t *this, unsigned int index);

#endif