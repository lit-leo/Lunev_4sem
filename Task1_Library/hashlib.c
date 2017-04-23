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
#include <string.h>
#include <assert.h>
#include "hashlib.h"
#define DBG_MODE

const static unsigned int align = sizeof(unsigned int) * 8;

struct bitArray
{
    unsigned int* array;
    unsigned int capacity;
};

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
    unsigned int calloc_size = 0;

    if((range % align) != 0)
        calloc_size = range / align + 1;
    else
    calloc_size = range / align;
        
    this->array = (unsigned int*)calloc(calloc_size, sizeof(unsigned int));
    if(this->array == NULL)
    {
        errno = ENOMEM;
        return -1;
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
    this->inSequence = NULL;
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

    this->inSequence = (bitArray_t*)(calloc(1, sizeof(bitArray_t)));
    if(bitArrayCtor(this->inSequence, this->capacity) < 0)
    {
        errno = ENOMEM;
        return -1;
    }

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

    bitArrayDtor(this->inSequence);
    free(this->inSequence);

    return 0;
}

unsigned int hashGetIndex(const char* string, unsigned int probe, unsigned int size)
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
    if(load_factor >= 0.75)
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
        index = hashGetIndex(data, probe, this->capacity);
        if((this->table)[index] == NULL)
        {
            (this->table)[index] = data;
            (this->used)++;
            bitArraySet(this->inSequence, index);
            
            return index;
        }
    }
    /*all probes were unsuccessful*/
    errno = ENOMEM;/*????is it correct??*/
    return -1;
}

/*fails to find old elements after size is doubled*/
/*because hashGetIndex returns another index sequence*/
int hashTableFind(hashTable_t *this, char *data)
{
    if(data == NULL)
    {
        errno = EINVAL;
        return -2;
    }

    unsigned int probe = 0;
    unsigned int index = 0;
    int data_len = strlen(data);
    for(; probe < this->capacity; probe++)
    {
        index = hashGetIndex(data, probe, this->capacity);
        if(!bitArrayTest(this->inSequence, index))
        {
        /*entry with this index is not a part of any sequence*/
            return -1;
        }

        if((this->table)[index] == NULL)
        /*enrty with this index has been cleared, moving on*/
            continue;

        int different = 0;
        if(data_len <= strlen((this->table)[index]))
            different = strncmp(data, (this->table)[index], data_len);
        else
            different = strncmp(data, (this->table)[index], strlen((this->table)[index]));

        if(!different)
        {   
            return index;
        }
    }

    /*no match found*/
    return -1;
}

int hashTableDelete(hashTable_t *this, char *data)
{
    int index = hashTableFind(this, data);
    /*if no element with matching string found*/
    if(index < 0)
        return -1;

    (this->table)[index] = NULL;
    this->used--;
    return 0;
}

int hashTableExpand(hashTable_t *this)
{
    unsigned int new_size = 2 * this->capacity;

    bitArray_t *new_array = NULL;
    new_array = (bitArray_t*)calloc(1, sizeof(bitArray_t));
    if(new_array == NULL)
    {
        errno = ENOMEM;
        return -1;
    }
    /*????should I set smth to errno or it'll derrive from Ctor?*/
    if(bitArrayCtor(new_array, new_size) != 0)
        return -1;

    char **new_table = (char**)calloc(new_size, sizeof(char*));
    if(new_table == NULL)
    {
        #ifdef DBG_MODE
        fprintf(stderr, "No expansion happened because of hashTable\n");
        #endif   

        errno = ENOMEM;
        return -1;
    }

    for(int i = 0; i < new_size; i++)
        new_table[i] = NULL;

    /*fill new_table*/
    for(int i = 0; i < this->capacity; i++)
        if(bitArrayTest(this->inSequence, i) && (this->table)[i] != NULL)
        {
            /*move string to a new position in new table*/
            unsigned int index = 0;
            for(unsigned int probe = 0; probe < new_size; probe++)
            {
                index = hashGetIndex((this->table)[i], probe, new_size);
                if(new_table[index] == NULL)
                {
                    new_table[index] = (this->table)[i];
                    bitArraySet(new_array, index);
                    break;
                }
            }
            /*???? should i check, that string is really moved?*/
        }

    bitArrayDtor(this->inSequence);
    free(this->inSequence);
    this->inSequence = new_array;
    this->capacity = new_size;
    free(this->table);
    this->table = new_table;

    #ifdef DBG_MODE
    fprintf(stderr, "\nInfo was called by %s;", __FUNCTION__);
    fprintf(stderr, "\nAfter expansion");
    hashTableInfo(this, stderr);
    #endif

    return 0;
}

void hashTableInfo(hashTable_t *this, FILE* stream)
{
    assert(hashTableVerify(this));
    fprintf(stream, "\n-----hashTableInfo of %p -----\n", this);
    fprintf(stream, "hashTable capacity: %u\n", this->capacity);
    fprintf(stream, "hashTable used: %u\n", this->used);
    fprintf(stream, "hashTable load_factor: %g\n", (float)this->used / (float)this->capacity);
    fprintf(stream, "bitArray: %p\n", this->inSequence);    
    fprintf(stream, "bitArray capacity: %u\n", this->inSequence->capacity);
    fprintf(stream, "bitArray structure:\n");
    for(int i = 0; i < this->inSequence->capacity; i++)
        fprintf(stream, "%u", bitArrayTest(this->inSequence, i));
    fprintf(stream, "\n");
    fprintf(stream, "hashTable structure:\n");
    for(int i = 0; i < this->capacity; i++)
        if(bitArrayTest(this->inSequence, i) && (this->table)[i] != NULL)
            fprintf(stream, "table[%d] = %s\n", i, (this->table)[i]);
    fprintf(stream, "--------------------------------\n");   
}

int hashTableVerify(hashTable_t *this)
{
    if((this == NULL) || 
    (this->table == NULL) ||
    (this->capacity <= 0) ||
    (this->used < 0)      ||
    (this->inSequence->capacity <= 0) ||
    (this->inSequence->capacity != this->capacity))
        return 0;
    else
        return 1;
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