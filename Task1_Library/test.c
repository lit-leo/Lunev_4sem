#include "hashlib.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    hashTable_t test;
    hashTableCtor(&test, 16);
    char ex[15] = "JA DISHU POD X";
    unsigned int i = hashTableInsert(&test, (char*)(&ex));
    
    printf("%s\n", (test.table)[i]);

    return 0;
}