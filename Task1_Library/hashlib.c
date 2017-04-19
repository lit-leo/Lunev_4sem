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
#include <assert.h>
#include "hashlib.h"
//#define DBG_MODE

const static unsigned int align = sizeof(unsigned int) * 8;

int hashTableExpand(hashTable_t *this);
void hashTableInfo(hashTable_t *this, FILE* stream);
int hashTableVerify(hashTable_t *this);

int bitArrayCtor(bitArray_t *this, unsigned int range)
{
    if(range <= 0)
    {
        errno = EINVAL;
        return -1;
    }

    this->capacity = range;
    this->array = NULL;

    if((range % align) != 0)
    {
        this->array = (unsigned int*)calloc(range / align + 1, sizeof(unsigned int));
        if(this->array == NULL)
        {
            errno = ENOMEM;
            return -1;
        }
    }
    else
    {
        this->array = (unsigned int*)calloc(range / align, sizeof(unsigned int));
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
    this->capacity = -1;
    free(this->array);

    return 0;
}

int bitArraySet(bitArray_t *this, unsigned int index)
{
    if(index < 0 || index >= this->capacity)
    {
        errno = EINVAL;
        return -1;
    }

    (this->array)[index / align] |= (1 << (index % align));

    return 0;
}

int bitArrayClear(bitArray_t *this, unsigned int index)
{
    if(index < 0 || index >= this->capacity)
    {
        errno = EINVAL;
        return -1;
    }

    (this->array)[index / align] &= (~(1 << index % align));

    return 0; 
}

int bitArrayTest(bitArray_t *this, unsigned int index)
{
    if(index < 0 || index >= this->capacity)
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
    /*if(1 < 0)
    {
        errno = ENOMEM;
        return -1;
    }*/

    #ifdef DBG_MODE
    fprintf(stderr, "\nInfo was called by %s;", __FUNCTION__);
    hashTableInfo(this, stderr);
    #endif

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
    if(data == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    #ifdef DBG_MODE
    fprintf(stderr, "\nInfo was called by %s;\n", __FUNCTION__);
    fprintf(stderr, "Before Insertion:");
    hashTableInfo(this, stderr);
    #endif

    float load_factor = (float)this->used / (float)this->capacity;
    if(load_factor >= 0.5)
    {
        #ifdef DBG_MODE
        fprintf(stderr, "load_factor is %g, expansion initiated\n", load_factor);
        #endif
        hashTableExpand(this);
    }

    unsigned int probe = 0;
    unsigned int index = 0;
    for(; probe < this->capacity; probe++)
    {
        index = hashFunc(data, probe, this->capacity);
        if((this->table)[index] == NULL)
        {
            (this->table)[index] = data;
            (this->used)++;
            bitArraySet(&(this->bitArray_inUse), index);
            
            return index;
        }
    }
    /*all probes were unsuccessful*/
    errno = ENOMEM;
    return -1;
}

void hashTableInfo(hashTable_t *this, FILE* stream)
{
    assert(hashTableVerify(this));
    fprintf(stream, "\n-----hashTableInfo of %p -----\n", this);
    fprintf(stream, "hashTable capacity: %u\n", this->capacity);
    fprintf(stream, "hashTable used: %u\n", this->used);
    fprintf(stream, "hashTable load_factor: %g\n", (float)this->used / (float)this->capacity);
    fprintf(stream, "bitArray capacity: %u\n", this->bitArray_inUse.capacity);
    fprintf(stream, "bitArray structure:\n");
    for(int i = 0; i < this->bitArray_inUse.capacity; i++)
        fprintf(stream, "%u", bitArrayTest(&(this->bitArray_inUse), i));
    fprintf(stream, "\n");
    fprintf(stream, "hashTable structure:\n");
    for (int i = 0; i < this->capacity; i++)
        if(bitArrayTest(&(this->bitArray_inUse), i) && (this->table)[i] != NULL)
            fprintf(stream, "table[%d] = %s\n", i, (this->table)[i]);
    fprintf(stream, "--------------------------------\n");
   
}

int hashTableVerify(hashTable_t *this)
{
    if((this == NULL) || 
    (this->table == NULL) ||
    (this->capacity <= 0) ||
    (this->used < 0)      ||
    (this->bitArray_inUse.capacity <= 0))
        return 0;
    else
        return 1;
}

/*!!! Mixture of new_range and *= 2*/
int bitArrayExpand(bitArray_t *this)
{
    unsigned int new_range = this->capacity * 2;
    
    /*check new range fits in existing space*/
    if(new_range <= align)
    {
        this->capacity *= 2;
        return 0;
    }    

    unsigned int realloc_size = 0;
    if((new_range % align) != 0)
        realloc_size = (new_range / align + 1) * sizeof(unsigned int);

    else
        realloc_size = new_range / align * sizeof(unsigned int);

    unsigned int *new_area = (unsigned int*)realloc(this->array, realloc_size);

    if(new_area == NULL)
    {
        errno = ENOMEM;

        #ifdef DBG_MODE
        fprintf(stderr, "Not Enough Memory during bitArrayExpand\n");
        #endif

        return -1;
    }

    this->array = new_area;
    this->capacity *= 2;

    return 0;
}

int hashTableExpand(hashTable_t *this)
{
    if(bitArrayExpand(&(this->bitArray_inUse)) != 0)
    {
        errno = ENOMEM;

        #ifdef DBG_MODE
        fprintf(stderr, "No expansion happened because of bitArray\n");
        #endif  

        return -1;
    }

    unsigned int new_size = 2 * this->capacity * sizeof(char*);
    char **new_area = (char**)realloc(this->table, new_size);

    if(new_area == NULL)
    {
        errno = ENOMEM;

        #ifdef DBG_MODE
        fprintf(stderr, "No expansion happened because of hashTable\n");
        #endif   

        return -1;
    }

    this->capacity *= 2;
    this->table = new_area;

    for (int i = this->capacity / 2; i < this->capacity; ++i) //!!!FIXME
    {
        (this->table)[i] = NULL;
    }

    #ifdef DBG_MODE
    fprintf(stderr, "\nInfo was called by %s;", __FUNCTION__);
    fprintf(stderr, "\nAfter expansion");
    hashTableInfo(this, stderr);
    #endif

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

unsigned int hashRot13_odd(const char * string)
{

    unsigned int hash = 0;

    for(; *string; string++)
    {
        hash += (unsigned char)(*string);
        hash -= (hash << 13) | (hash >> 19);
    }

    return hash | 1;

}

unsigned int hashLY(const char* string)
{
    unsigned int hash = 0;

    for(; *string; string++)
        hash = (hash * 1664525) + (unsigned char)(*string) + 1013904223;

    return hash;

}

unsigned int hashLY_odd(const char* string)
{
    unsigned int hash = 0;

    for(; *string; string++)
        hash = (hash * 1664525) + (unsigned char)(*string) + 1013904223;

    return hash | 1;

}