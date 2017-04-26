#include "hashlib.h"
#include "bitarraylib.h"
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

#define SETMEMLIM(value)\
    struct rlimit rlim_old;\
    struct rlimit rlim;\
    getrlimit(RLIMIT_AS, &rlim_old);\
    rlim.rlim_cur = value;\
    rlim.rlim_max = rlim_old.rlim_max;\
    setrlimit(RLIMIT_AS, &rlim)
    //printf("SETMEMLIMIT %d\n", setrlimit(RLIMIT_AS, &rlim))

#define RESTOREMEMLIM()\
        setrlimit(RLIMIT_AS, &rlim_old)

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

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

    SETMEMLIM(1024 * 1024);
    retv = hashTableCtor(&test, 65536/*8193*/);
    CHECK_RETV_ERRNO(retv, -1, ENOMEM);
    RESTOREMEMLIM();

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
    char dummy[32] = "THIS IS DUMMY TO TEST EXPANSION";
    hashTable_t test;
    int retv =  hashTableCtor(&test, 16);
    CHECK_RETV(retv, 0);
    
    /*successful expansion*/
    /*forcing to intitiate expansion during next insertion*/
    test.used = 12;
    retv = hashTableInsert(&test, (char*)dummy);
    if(retv < 0 || test.capacity != 32 ||
        (test.inSequence)->capacity != 32)
        return 0;

    retv = hashTableDtor(&test);
    CHECK_RETV(retv, 0);

    /*unsuccsessful expansion*/
    SETMEMLIM(5 * 1024 * 1024);
    retv = hashTableCtor(&test, 65536);
    CHECK_RETV(retv, 0);

    /*forcing to intitiate expansion during next insertion*/
    test.used = 49153;
    retv = hashTableInsert(&test, (char*)dummy);
    if(retv < 0 || test.capacity != 65536 ||
        (test.inSequence)->capacity != 65536)
        return 0;
    RESTOREMEMLIM();

    retv = hashTableDtor(&test);
    CHECK_RETV(retv, 0);

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
    char ex2[32] = "SECOND EXAMPLE";
    char ex3[32] = "Example to not to be found";

    if(hashTableFind(&test, (char*)ex1) >= 0)
        return 0;

    int index = hashTableInsert(&test, (char*)ex1);
    if(index < 0)
        return 0;
    if(hashTableFind(&test, (char*)ex1) != index)
        return 0;
    /*force to not ot expand to get full table*/
    test.used = 0;

    index = hashTableInsert(&test, (char*)ex2);
    if(index < 0)
        return 0;
    if(hashTableFind(&test, (char*)ex2) != index)
        return 0;
    /*force to not to expand to get full table*/
    test.used = 0;

    retv = hashTableFind(&test, (char*)ex3);
    CHECK_RETV(retv, -1);
    /*restore normal test.used value*/
    test.used = 2;

    retv = hashTableDelete(&test, (char*)ex1);
    CHECK_RETV(retv, 0);
    if(hashTableFind(&test, (char*)ex1) != -1)
        return 0;

    /*force table to exapand and rehash*/
    for (int i = 0; i < 32; ++i)
    {
        retv = hashTableInsert(&test, (char*)ex1);
        if(retv < 0)
            return 0;
    }
    if(hashTableFind(&test, (char*)ex2) < 0 ||
        hashTableFind(&test, (char*)ex1) < 0)
        return 0;

    return 1;


}

int testHashTableDelete()
{
    int retv = hashTableDelete(NULL, NULL);
    CHECK_RETV_ERRNO(retv, -2, EINVAL);

    hashTable_t table;
    retv = hashTableCtor(&table, 16);
    CHECK_RETV(retv, 0);

    char ex[32] = "Example.";
    retv = hashTableDelete(&table, (char*)ex);
    CHECK_RETV(retv, -1);

    int index = hashTableInsert(&table, (char*)ex);
    if(index < 0)
        return 0;
    retv = hashTableDelete(&table, (char*)ex);
    CHECK_RETV(retv, 0);

    return 1;
}
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
    PRTEST(testHashTableExpand());
    PRTEST(testHashTableFind());
    PRTEST(testHashTableDelete());
    /*PRTEST(testHashTableIteratorCtor());
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
}*/

/*int nullComponentsTest()
{
    int passed = 0;
    hashTable_t test_table;
    char example[32] = "test";

    hashTableCtor(NULL, 1);
    hashTableCtor(&test_table, -1);
    hashTableCtor(&test_table, 1024 * 16);
    hashTableInfo(&test_table, stderr);

    hashTableInsert(NULL, NULL);
    hashTableInsert(&test_table, NULL);
    hashTableInsert(NULL, (char *)example);

    hashTableDtor(NULL);
    hashTableDtor(&test_table);

    return passed;

}*/