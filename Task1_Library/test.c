#include "hashlib.h"
#include <stdio.h>
#include <stdlib.h>

int commonTest();
int nullComponentsTest();

int main(int argc, char** argv)
{
    commonTest();

    nullComponentsTest();

    return 0;
}

int commonTest()
{
    int passed = 0;

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
    hashTableCtor(&test, 4);
    for (int i = 0; i < 14; ++i)
    {
        hashTableInsert(&test, test_strings[i]);
    }
    hashTableInfo(&test, stderr);
    if(hashTableFind(&test, (char*)findExample) >= 0)
        fprintf(stderr, "Find -- Success\n");

    for (int i = 0; i < 14; ++i)
    {
        hashTableDelete(&test, test_strings[i]);
    }
    hashTableInfo(&test, stderr);
    hashTableDtor(&test);

    fclose(filein);

    return passed;
}

int nullComponentsTest()
{
    int passed = 0;
    hashTable_t test_table;
    char example[32] = "test";

    /**** Ctor ****/
    hashTableCtor(NULL, 1);
    hashTableCtor(&test_table, -1);
    hashTableCtor(&test_table, 1);

    /**** Insert ****/
    hashTableInsert(NULL, NULL);
    hashTableInsert(&test_table, NULL);
    hashTableInsert(NULL, (char *)example);

    hashTableDtor(NULL);
    hashTableDtor(&test_table);

    return passed;

}