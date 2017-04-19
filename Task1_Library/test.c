#include "hashlib.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    char ex1[32] = "THIS IS TEST STRING";
    char ex2[32] = "YET ANOTHER TEST STRING";
    hashTable_t test;
    hashTableCtor(&test, 1);

    for (int i = 0; i < atoi(argv[1]); ++i)
    {
        ex1[i % 32] += (i % 3);
        hashTableInsert(&test, (char*)ex1);
    }

    //printf("%s\n", NULL/*(test.table)[5]*/);

    for (int i = 0; i < atoi(argv[1]); ++i)
    {
        ex2[i % 32] += (i % 3);
        hashTableInsert(&test, (char*)ex2);
    }

    hashTableInfo(&test, stderr);

    hashTableDtor(&test);



    return 0;
}