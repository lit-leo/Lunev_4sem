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

const static unsigned int align = sizeof(unsigned int) * 8;

int bitArrayCtor(bitArray_t *this, unsigned int range)
{
    if(range <= 0)
    {
        errno = EINVAL;
        return -1;
    }

    this->size = range;
    this->array = NULL;

    if((range % 4) != 0)
    {
        this->array = (unsigned int*)calloc(range / sizeof(unsigned int) + 1, sizeof(unsigned int));
        if(this->array == NULL)
        {
            errno = ENOMEM;
            return -1;
        }
    }
    else
    {
        this->array = (unsigned int*)calloc(range / sizeof(unsigned int), sizeof(unsigned int));
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
    if(index < 0 || index >= this->size)
    {
        errno = EINVAL;
        return -1;
    }

    (this->array)[index / align] |= (1 << (index % align));

    return 0;
}

int bitArrayClear(bitArray_t *this, unsigned int index)
{
    if(index < 0 || index >= this->size)
    {
        errno = EINVAL;
        return -1;
    }

    (this->array)[index / align] &= (~(1 << index % align));

    return 0; 
}

int bitArrayTest(bitArray_t *this, unsigned int index)
{
    if(index < 0 || index >= this->size)
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

/*!!!DEBUG NEEDED. CALLOC CALLS ARE WRONG!!!*/
int hashTableCtor(hashTable_t *this, unsigned int size)
{
    int sizeIsDoubled = 1;
    this->used = 0;
    this->table = NULL;
    unsigned int range = nearest2pwr(size * 2);

    this->table = (char**)calloc(range, sizeof(char*));
    if (this->table == NULL)
    {
        range /= 2;
        sizeIsDoubled = 0;

        this-> table = (char**)calloc(range, sizeof(char*));
    }
    if (this->table == NULL)
    {
        errno = ENOMEM;
        return -1;
    }

    this->capacity = range;
    for(int i = 0; i < this->capacity; i++)
        (this->table)[i] = NULL;

    bitArrayCtor(&(this->bitArray_inUse), this->capacity);
    if(1/**/ < 0)
    {
        errno = ENOMEM;
        return -1;
    }

    return sizeIsDoubled;
}

int hashTableDtor(hashTable_t *this)
{
    this->used = -1;
    this->capacity = -1;
    free(this->table);
    
    /*nothing to check*/
    bitArrayDtor(&(this->bitArray_inUse));

    return 0;
}

unsigned int hashFunc(const char* string, unsigned int probe, unsigned int size)
{
    return (hashRot13(string) + probe * hashLY_odd(string)) % size;
}

int hashTableInsert(hashTable_t *this, char* data)
{
    unsigned int probe = 0;
    unsigned int index = 0;
    for(; probe < this->capacity; probe++)
    {
        index = hashFunc(data, probe, this->capacity);
        if((this->table)[index] == NULL)
        {
            (this->table)[index] = data;

            bitArraySet(&(this->bitArray_inUse), index);
            
            return index;
        }
    }

    errno = ENOMEM;
    return -1;
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