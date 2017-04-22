#include "hashlib.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    char example[32] = "Hi, It's me, Mario!";
    hashTable_t test;
    hashTableCtor(&test, 1);
    for (int i = 0; i < 3; ++i)
    {
        hashTableInsert(&test, (char*)example);
    }
    hashTableInfo(&test, stdout);
    for (int i = 0; i < 3; ++i)
    {
        hashTableDelete(&test, (char*)example);
    }
    hashTableInfo(&test, stdout);
    hashTableDtor(&test);
    return 0;
}