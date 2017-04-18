#include "hashlib.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    /*hashTable_t test;
    hashTableCtor(&test, 19000);
    unsigned int i = hashTableInsert(&test, (char*)(&ex));
    
    printf("%s\n", (test.table)[i]);*/
    char ex[15] = "JA DISHU POD X";

    int top = 2097152;
    bitArray_t indicator;
    bitArrayCtor(&indicator, top);
    for(int i = 0; i < top; i++)
        bitArraySet(&indicator, hashFunc((char*)(&ex), i, top));

    for (int i = 0; i < indicator.size; ++i)
    {
        if(bitArrayTest(&indicator, i) == 0)
            printf("CHEEKE BREEKE I V DAMKE\n");
    }
    return 0;
}