#include "hashlib.h"

unsigned int hashH37(const char * string)
{
    unsigned int hash = 2139062143;

    for(; *string; string++)
        hash = 37 * hash + *string;

    return hash;
}

unsigned int hashLY(const char* string)
{
    unsigned int hash = 0;

    for(; *string; string++)
        hash = (hash * 1664525) + (unsigned char)(*string) + 1013904223;

    return hash;

}