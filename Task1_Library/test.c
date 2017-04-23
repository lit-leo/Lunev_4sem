#include "hashlib.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    char test_strings[14][64];
    char findExample[64] = "And what was once his bliss appears his bane.";
    FILE *filein = fopen("stringsForTests.txt", "r");
    
    if(filein == NULL)
    {
        perror("Input file");
        exit(2);
    }

    for (int i = 0; i < 14; ++i)
    {
        fgets(test_strings[i], 64, filein);
    }
/******************************************************************************/

    hashTable_t test;
    hashTableCtor(&test, 1);
    for (int i = 0; i < 14; ++i)
    {
        hashTableInsert(&test, test_strings[i]);
    }
    hashTableInfo(&test, stdout);
    printf("%d\n", hashTableFind(&test, (char*)findExample));
    for (int i = 0; i < 14; ++i)
    {
        hashTableDelete(&test, test_strings[i]);
    }
    hashTableInfo(&test, stdout);
    hashTableDtor(&test);

    return 0;
}