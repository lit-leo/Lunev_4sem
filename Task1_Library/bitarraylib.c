#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitarraylib.h"

const static unsigned int align = sizeof(unsigned int) * 8;

int bitArrayCtor(bitArray_t *this, unsigned int range)
{
    if(this == NULL || range <= 0)
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
    if(this != NULL)
        this->capacity = -1;

    free(this->array);

    return 0;
}

int bitArraySet(bitArray_t *this, unsigned int index)
{
    if(this == NULL || 
      (index < 0 || index >= this->capacity))
    {
        errno = EINVAL;
        return -1;
    }

    (this->array)[index / align] |= (1 << (index % align));

    return 0;
}

int bitArrayTest(bitArray_t *this, unsigned int index)
{
    if(this == NULL || 
      (index < 0 || index >= this->capacity))
    {
        errno = EINVAL;
        return -1;
    }

    if((this->array[index / align] & (1 << index % align)) != 0)
        return 1;
    else
        return 0;     
}
