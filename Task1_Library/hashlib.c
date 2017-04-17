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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "hashlib.h"

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

    //security needed!!!!
    bitArrayCtor(this->inUse, range);

    return sizeIsDoubled;
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