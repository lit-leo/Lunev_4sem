/**********************************************
* FILENAME:     hashlib.c
*
* DESCRIPTION:
*       Hash table with open adressing routines.
* 
* AUTHOR: Leonid Litovchenko
*
* START DATE: 17 APR 2017
*
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "hashlib.h"

const static unsigned int align = sizeof(unsigned int) *8;

/*ПЕРЕПИСАТЬ!!! SIZE ПОТЕНЦИАЛЬНО НЕВЕРНО ОПРЕДЕЛЁН*/
int bitArrayCtor(bitArray_t *this, unsigned int size)
{
    if(size <= 0)
    {
        errno = EINVAL;
        return -1;
    }

    this->size = size;

    this->array = NULL;

    if(size % 4 != 0)
    {
        this->array = calloc(size / sizeof(unsigned int) + 1, sizeof(unsigned int));
        if(this->array == NULL)
        {
            errno = ENOMEM;
            return -1;
        }
    }
    else
    {
        this->array = calloc(size / sizeof(unsigned int), sizeof(unsigned int));
        if(this->array == NULL)
        {
            errno = ENOMEM;
            return -1;
        }
    }

    return 0;
}

int bitArrayDtor(bitArray_t *this)
{
    this->size = -1;
    free(this->array);

    return 0;
}

int bitArraySet(bitArray_t *this, unsigned int index)
{
    if(index < 0)
    {
        errno = EINVAL;
        return -1;
    }

    (this->array)[index / align] |= (1 << (index % align));

    return 0;
}

int bitArrayClear(bitArray_t *this, unsigned int index)
{
    if(index < 0)
    {
        errno = EINVAL;
        return -1;
    }

    (this->array)[index / align] &= (~(1 << index % align));

    return 0; 
}

int bitArrayTest(bitArray_t *this, unsigned int index)
{
    if(index < 0)
    {
        errno = EINVAL;
        return -1;
    }

    if((this->array[index / align] & (1 << index % align)) != 0)
        return 1;
    else
        return 0;     
}

unsigned int nearest2pwr(unsigned int value)
{
    unsigned int i = 1;
    for(; value > i; i*=2);

    return i;
}

int hashTableCtor(hashTable_t *this, unsigned int size)
{
    int sizeIsDoubled = 1;
    this->used = 0;
    this->table = NULL;
    unsigned int range = nearest2pwr(size * 2);
    /*превратить size в ближайшую к 2*size степень 2*/
    this->table = calloc(range, sizeof(char*));
    if (this->table == NULL)
    {
        range /= 2;
        sizeIsDoubled = 0;

        this-> table = calloc(range, sizeof(char*));
    }
    if (this->table == NULL)
    {
        errno = ENOMEM;
        return -1;
    }

    this->capacity = range;

    /*security check needed*/
    bitArrayCtor(this->bitArray_inUse, range);
    return sizeIsDoubled;
}

int hashTableDtor(hashTable_t *this)
{
    this->used = -1;
    this->capacity = -1;
    free(this->table);
    
    /*security check needed*/
    bitArrayDtor(this->bitArray_inUse);

    return 0;
}

int expand()
{
    /*<<<< Expands the table >>>>*/
    return 0;
}

unsigned int hashRot13(const char * string)
{

    unsigned int hash = 0;

    for(; *string; string++)
    {
        hash += (unsigned char)(*string);
        hash -= (hash << 13) | (hash >> 19);
    }

    return hash;

}

unsigned int hashLY_odd(const char* string)
{
    unsigned int hash = 0;

    for(; *string; string++)
        hash = (hash * 1664525) + (unsigned char)(*string) + 1013904223;

    return hash | 1;

}