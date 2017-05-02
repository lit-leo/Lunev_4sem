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
#include "hashlib.h"
#include "bitarraylib.h"
//#define DBG_MODE

/****
* Returns power of 2 that is bigger than value.
*/
unsigned int nearest2pwr(unsigned int value);

struct hashTable
{
    unsigned int capacity;
    unsigned int used;
    bitArray_t *inSequence;
    char** table;
};

struct hashTableIterator
{
    hashTable_t *hashTable;
    int currentIndex;
};

int hashTableIteratorCtor(hashTableIterator_t *this, hashTable_t *from)
{
    if(this == NULL ||
        from == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    this->hashTable = from;
    this->currentIndex = -1;

    return 0;
}

char *hashTableIteratorFirst(hashTableIterator_t *this)
{
    if(this == NULL ||
        this->hashTable == NULL)
    {
        errno = EINVAL;
        return NULL;
    }

    for (int i = 0; i < this->hashTable->inSequence->capacity; ++i)
        if(bitArrayTest(this->hashTable->inSequence, i) &&
          (this->hashTable->table)[i] != NULL)
            {
                this->currentIndex = i;
                return (this->hashTable->table)[i];
            }

    /*Nothing found*/
    return NULL;
}

char *hashTableIteratorNext(hashTableIterator_t *this)
{
    if(this == NULL ||
        this->hashTable == NULL)
    {
        errno = EINVAL;
        return NULL ;
    }

    for (int i = this->currentIndex + 1; i < this->hashTable->inSequence->capacity; ++i)
        if(bitArrayTest(this->hashTable->inSequence, i) &&
          (this->hashTable->table)[i] != NULL)
            {
                this->currentIndex = i;
                return (this->hashTable->table)[i];
            }

    /*Nothing found*/
    return NULL;
}

int hashTableIteratorIsLast(hashTableIterator_t *this)
{
    if(this == NULL ||
        this->hashTable == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    for (int i = this->currentIndex + 1; i < this->hashTable->inSequence->capacity; ++i)
        if(bitArrayTest(this->hashTable->inSequence, i) &&
          (this->hashTable->table)[i] != NULL)
                return 0;

    /*No more elements met*/
    return 1;
}

char *hashTableIteratorGet(hashTableIterator_t *this)
{
    if(this == NULL ||
        this->hashTable == NULL ||
       this->currentIndex < 0)
    {   
        errno = EINVAL;
        return NULL;
    }

    return (this->hashTable->table)[this->currentIndex];
        
}

int hashTableExpand(hashTable_t *this);
#ifdef DBG_MODE
void hashTableInfo(hashTable_t *this, FILE* stream);
#endif
int hashTableVerify(hashTable_t *this);
unsigned int hashRot13(const char * string);
unsigned int hashLY_odd(const char* string);


int hashTableCtor(hashTable_t *this, unsigned int size)
{
    /*second check is to ensure that size is positive*/
    if(this == NULL ||
        (int)size <= 0)
    {
        errno = EINVAL;
        return -1;
    }

    this->used = 0;
    this->table = NULL;
    this->inSequence = NULL;
    unsigned int range = nearest2pwr(size);

    this->table = (char**)calloc(range, sizeof(char*));
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
        return -1;

    #ifdef DBG_MODE
    fprintf(stderr, "\nInfo was called by %s;", __FUNCTION__);
    hashTableInfo(this, stderr);
    #endif

    return 0;
}

int hashTableDtor(hashTable_t *this)
{
    if(this == NULL)
    {
        errno = EINVAL;
        return -1;
    }

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
    if(!hashTableVerify(this) ||
        data == NULL)
    {
        errno = EINVAL;
        return -2;
    }

    #ifdef DBG_MODE
    fprintf(stderr, "\nInfo was called by %s;\n", __FUNCTION__);
    fprintf(stderr, "Before Insertion:");
    hashTableInfo(this, stderr);
    #endif

    if(this->used * 1024 >= this->capacity * 750)
    {
        #ifdef DBG_MODE
        fprintf(stderr, "load_factor exceeded, expansion initiated\n");
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
    return -1;
}

int hashTableFind(hashTable_t *this, char *data)
{
    if(!hashTableVerify(this) ||
        data == NULL)
    {
        errno = EINVAL;
        return -2;
    }

    unsigned int index = 0;
    int data_len = strlen(data);
    for(unsigned int probe = 0; probe < this->capacity; probe++)
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

        /*lengths don't match*/
        if(data_len != strlen((this->table)[index]))
            continue;

        if(!strncmp(data, (this->table)[index], data_len))
            return index;
    }

    /*no match found*/
    return -1;
}

int hashTableDelete(hashTable_t *this, char *data)
{
    if(!hashTableVerify(this) ||
        data == NULL)
    {
        errno = EINVAL;
        return -2;
    }

    /**/
    unsigned int index = 0;
    unsigned int probe = 0;
    int data_len = strlen(data);
    for(; probe < this->capacity; probe++)
    {
        index = hashGetIndex(data, probe, this->capacity);
        if(!bitArrayTest(this->inSequence, index))
        /*entry with this index is not a part of any sequence*/
            return -1;

        if((this->table)[index] == NULL)
        /*enrty with this index has been cleared, moving on*/
            continue;

        /*lengths don't match*/
        if(data_len != strlen((this->table)[index]))
            continue;

        if(!strncmp(data, (this->table)[index], data_len))
        /*correspondent string found!*/
            break;
    }
    /*no correspondent string found*/
    if(probe == this->capacity)
        return -1;

    (this->table)[index] = NULL;
    this->used--;

    int filled_ahead = 0;
    /*go forward and check, if there is filled entry ahead*/
    for(unsigned int i = probe + 1; i < this->capacity; i++)
    {
        unsigned int index = hashGetIndex(data, i, this->capacity);
        if(bitArrayTest(this->inSequence, index))
        {
            filled_ahead++;
            break;
        }
    }

    if(!filled_ahead)
    {
        bitArrayClear(this->inSequence, index);
        /*if nothing ahead, 
        * go backwards trying to find filled entry while cleaning up 
        * inSequence bit array.
        */
        for(unsigned int i = probe - 1; (int)i >= 0; i--)
        {
            unsigned int index = hashGetIndex(data, i, this->capacity);
            if(bitArrayTest(this->inSequence, index))
            {
                if((this->table)[index] ==NULL)
                    bitArrayClear(this->inSequence, index);
                else
                    break;          
            }

        }
    }

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

#ifdef DBG_MODE
void hashTableInfo(hashTable_t *this, FILE* stream)
{
    if(!hashTableVerify(this))
    {
        fprintf(stream, "\n-----hashTableInfo ERROR -----\n");
        return;
    }
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
#endif

int hashTableVerify(hashTable_t *this)
{
    if((this == NULL) || 
    (this->table == NULL) ||
    (this->capacity <= 0) ||
    (this->used < 0)      ||
    (this->inSequence == NULL) ||
    (this->inSequence->capacity <= 0))
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

unsigned int nearest2pwr(unsigned int value)
{
    unsigned int i = 1;
    for(; value > i; i*=2);

    return i;
}