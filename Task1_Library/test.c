#include "hashlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define RESET   "\033[0m"
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */

#define CHECK_RETV_ERRNO(retv, val, err) \
    do { \
        if(!(retv == val && err == errno)) \
            return 0;\
        } while (0)

#define CHECK_RETV(retv, val) \
    do { \
        if(retv != val) \
            return 0;\
        } while (0)

#define PRTEST(func) \
    do {\
        if(func == 1)\
            printf("[" GREEN "OK" RESET "] "#func"\n");\
        else \
            printf("[" RED "FAILED" RESET"] "#func" \n");\
    } while(0)

#include <sys/time.h>
#include <sys/resource.h>

int commonTest();
int nullComponentsTest();


/*!!!! mem_limits are not working!*/
int testHashTableCtor()
{
    hashTable_t test;
    int retv = hashTableCtor(NULL, 15);
    CHECK_RETV_ERRNO(retv, -1, EINVAL);

    retv = hashTableCtor(&test, -1);
    CHECK_RETV_ERRNO(retv, -1, EINVAL);

    retv = hashTableCtor(&test, 1);
    CHECK_RETV(retv, 0);
    hashTableDtor(&test);

    struct rlimit rlim_old;
    struct rlimit rlim;
    rlim.rlim_cur = 1;
    //rlim.rlim_max = rlim_old.rlim_max;
    getrlimit(RLIMIT_AS, &rlim_old);
    setrlimit(RLIMIT_AS, &rlim);

    retv = hashTableCtor(&test, 20480000);
    CHECK_RETV_ERRNO(retv, -1, ENOMEM);

    setrlimit(RLIMIT_AS, &rlim_old);

    return 1;
}

int testHashTableDtor()
{
    int retv = hashTableDtor(NULL);
    CHECK_RETV_ERRNO(retv, -1, EINVAL);

    hashTable_t test;
    hashTableCtor(&test, 1024);
    retv = hashTableDtor(&test);
    CHECK_RETV(retv, 0);

    return 1;
}
int testHashTableInsert()
{
    char test_string[32] = "THIS IS ORDINARY TEST STRING";
    int retv = hashTableInsert(NULL, (char*)test_string);
    CHECK_RETV_ERRNO(retv, -2, EINVAL);

    hashTable_t test;
    retv = hashTableCtor(&test, 1);
    CHECK_RETV(retv, 0);
    retv = hashTableInsert(&test, NULL);
    CHECK_RETV_ERRNO(retv, -2, EINVAL);

    retv = hashTableInsert(&test, (char*)test_string);
    CHECK_RETV(retv, 0);
    
    /*disabling expansion*/
    test.used = 0;
    retv = hashTableInsert(&test, (char*)test_string);
    CHECK_RETV(retv, -1);

    return 1;
}
int testHashTableExpand()
{
    return 1;
}
int testHashTableFind()
{
    int retv = hashTableFind(NULL, NULL);
    CHECK_RETV_ERRNO(retv, -2, EINVAL);

    hashTable_t test;
    retv = hashTableCtor(&test, 2);
    CHECK_RETV(retv, 0);

    char ex1[32] = "THIS IS FIRST EXAMPLE";
    char ex2[32] = "THIS IS SECOND EXAMPLE";
    retv = hashTableInsert(&test, (char*)ex1);
    if(retv < 0)
        return 0;


}
int testHashTableDelete();
int testHashTableIteratorCtor();
int testHashTableIteratorFirst();
int testHashTableIteratorNext();
int testHashTableIteratorIsLast();
int testHashTableIteratorGet();

int main(int argc, char** argv)
{
    PRTEST(testHashTableCtor());
    PRTEST(testHashTableDtor());
    PRTEST(testHashTableInsert());
    /*PRTEST(testHashTableExpand());
    PRTEST(testHashTableFind());
    PRTEST(testHashTableDelete());
    PRTEST(testHashTableIteratorCtor());
    PRTEST(testHashTableIteratorFirst());
    PRTEST(testHashTableIteratorNext());
    PRTEST(testHashTableIteratorIsLast());
    PRTEST(testHashTableIteratorGet());*/

    return 0;
}

/*int commonTest()
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

    /*hashTable_t test;
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
}*/

/*int nullComponentsTest()
{
    int passed = 0;
    hashTable_t test_table;
    char example[32] = "test";

    /*hashTableCtor(NULL, 1);
    hashTableCtor(&test_table, -1);
    hashTableCtor(&test_table, 1024 * 16);
    hashTableInfo(&test_table, stderr);

    /*hashTableInsert(NULL, NULL);
    hashTableInsert(&test_table, NULL);
    hashTableInsert(NULL, (char *)example);

    hashTableDtor(NULL);
    hashTableDtor(&test_table);

    return passed;

}*/