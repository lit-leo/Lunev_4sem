#include "hashlib.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    char ex1[32] = "THIS IS TEST STRING";
    char ex2[32] = "YET ANOTHER TEST STRING";
    hashTable_t test;
    hashTableCtor(&test, 1);
    hashTableInsert(&test, (char*)ex1);
    hashTableInsert(&test, (char*)ex2);
    hashTableInsert(&test, (char*)ex1);

    hashTableInfo(&test, stdout);

    return 0;
}