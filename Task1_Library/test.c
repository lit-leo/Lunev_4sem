#include "hashlib.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    hashTable_t test;
    hashTableCtor(&test, 16);
    
    printf("%p\n", (test.table)[4]);

    return 0;
}