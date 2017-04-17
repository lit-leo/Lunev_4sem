#include "hashlib.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    char teststr [64];
    scanf("%s", teststr);
    printf("%u\n", hashH37(teststr));

    return 0;
}