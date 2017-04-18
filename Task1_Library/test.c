#include "hashlib.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    bitArray_t test;

    bitArrayCtor(&test, 34);
    printf("%u\n", test.size);
    printf("Set = %d\n", bitArraySet(&test, 33));
    bitArraySet(&test, 98);
    //int a = 1 << 2;
    printf("Test3 = %d\n", bitArrayTest(&test, 33));
    printf("Test2 = %d\n", bitArrayTest(&test, 98));
    printf("Test1 = %d\n", bitArrayTest(&test, 1));

    return 0;
}