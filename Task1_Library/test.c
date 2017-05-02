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

struct hashTableIterator
{
    hashTable_t *hashTable;
    int currentIndex;
};

struct hashTable
{
    unsigned int capacity;
    unsigned int used;
    bitArray_t *inSequence;
    char** table;
};

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

    char ex0[48] = "In mind a slave to every vicious joy;";
    for (int i = 0; i < 10; ++i)
    {
        int index = hashTableInsert(&table, (char*)ex0);
        if(index < 0)
            return 0;
    }

    for (int i = 0; i < 10; ++i)
    {
        int retv = hashTableDelete(&table, (char*)ex0);
        CHECK_RETV(retv, 0);
    }
    /*check that inSequence is clear when the table is clear*/
    for (int i = 0; i < table.inSequence->capacity; ++i)
    {
        if(bitArrayTest(table.inSequence, i))
            return 0;
    }

    return 1;
}
int testHashTableIteratorCtor()
{
    hashTableIterator_t tableIterator;
    int retv = hashTableIteratorCtor(NULL, NULL);
    CHECK_RETV_ERRNO(retv, -1, EINVAL);

    retv = hashTableIteratorCtor(&tableIterator, NULL);
    CHECK_RETV_ERRNO(retv, -1, EINVAL);    
    
    hashTable_t table;
    retv = hashTableCtor(&table, 16);
    CHECK_RETV(retv, 0);

    retv = hashTableIteratorCtor(&tableIterator, &table);
    if(retv != 0 ||
        tableIterator.hashTable != &table ||
        tableIterator.currentIndex != -1)
        return 0;

    return 1;
}
int testHashTableIteratorFirst()
{
    char *retp = hashTableIteratorFirst(NULL);
    CHECK_RETV_ERRNO(retp, NULL, EINVAL);

    hashTable_t table;
    int retv = hashTableCtor(&table, 16);
    CHECK_RETV(retv, 0);

    hashTableIterator_t tableIterator;
    retv = hashTableIteratorCtor(&tableIterator, &table);
    CHECK_RETV(retv, 0);

    retp = hashTableIteratorFirst(&tableIterator);
    CHECK_RETV(retp, NULL);

    char ex8[48] = "In law an infant, and in years a boy,";
    char ex0[48] = "In mind a slave to every vicious joy;";
    char ex13[48] = "From every sense of shame and virtue wean'd,";
    char ex11[48] = "In lies an adept, in deceit a fiend;";
    retv = hashTableInsert(&table, (char*)ex8);
    CHECK_RETV(retv, 8);
    retv = hashTableInsert(&table, (char*)ex0);
    CHECK_RETV(retv, 0);
    retv = hashTableInsert(&table, (char*)ex13);
    CHECK_RETV(retv, 13);
    retv = hashTableInsert(&table, (char*)ex11);
    CHECK_RETV(retv, 11);

    retp = hashTableIteratorFirst(&tableIterator);
    CHECK_RETV(retp, (char*)ex0);

    retv = hashTableDelete(&table, (char*)ex0);
    CHECK_RETV(retv, 0);

    retp = hashTableIteratorFirst(&tableIterator);
    CHECK_RETV(retp, (char*)ex8);

    retv = hashTableDtor(&table);
    CHECK_RETV(retv, 0);

    retp = hashTableIteratorFirst(NULL);
    CHECK_RETV_ERRNO(retp, NULL, EINVAL);

    return 1;
}
int testHashTableIteratorNext()
{
    char *retp = hashTableIteratorNext(NULL);
    CHECK_RETV_ERRNO(retp, NULL, EINVAL);

    hashTable_t table;
    int retv = hashTableCtor(&table, 16);
    CHECK_RETV(retv, 0);

    hashTableIterator_t tableIterator;
    retv = hashTableIteratorCtor(&tableIterator, &table);
    CHECK_RETV(retv, 0);

    retp = hashTableIteratorNext(&tableIterator);
    CHECK_RETV(retp, NULL);

    char ex8[48] = "In law an infant, and in years a boy,";
    char ex0[48] = "In mind a slave to every vicious joy;";
    char ex13[48] = "From every sense of shame and virtue wean'd,";
    char ex11[48] = "In lies an adept, in deceit a fiend;";
    retv = hashTableInsert(&table, (char*)ex8);
    CHECK_RETV(retv, 8);
    retv = hashTableInsert(&table, (char*)ex0);
    CHECK_RETV(retv, 0);
    retv = hashTableInsert(&table, (char*)ex13);
    CHECK_RETV(retv, 13);
    retv = hashTableInsert(&table, (char*)ex11);
    CHECK_RETV(retv, 11);

    retp = hashTableIteratorNext(&tableIterator);
    CHECK_RETV(retp, (char*)ex0);

    retp = hashTableIteratorNext(&tableIterator);
    CHECK_RETV(retp, (char*)ex8);

    retv = hashTableDelete(&table, (char*)ex11);
    CHECK_RETV(retv, 0);

    retp = hashTableIteratorNext(&tableIterator);
    CHECK_RETV(retp, (char*)ex13);

    retv = hashTableDtor(&table);
    CHECK_RETV(retv, 0);

    return 1;
}
int testHashTableIteratorIsLast()
{
    int retv = hashTableIteratorIsLast(NULL);
    CHECK_RETV_ERRNO(retv, -1, EINVAL);

    hashTable_t table;
    retv = hashTableCtor(&table, 16);
    CHECK_RETV(retv, 0);

    char ex8[48] = "In law an infant, and in years a boy,";
    char ex0[48] = "In mind a slave to every vicious joy;";
    char ex13[48] = "From every sense of shame and virtue wean'd,";
    char ex11[48] = "In lies an adept, in deceit a fiend;";
    retv = hashTableInsert(&table, (char*)ex8);
    CHECK_RETV(retv, 8);
    retv = hashTableInsert(&table, (char*)ex0);
    CHECK_RETV(retv, 0);
    retv = hashTableInsert(&table, (char*)ex13);
    CHECK_RETV(retv, 13);
    retv = hashTableInsert(&table, (char*)ex11);
    CHECK_RETV(retv, 11);

    hashTableIterator_t tableIterator;
    retv = hashTableIteratorCtor(&tableIterator, &table);
    CHECK_RETV(retv, 0);

    retv = hashTableIteratorIsLast(&tableIterator);
    CHECK_RETV(retv, 0);
   
    char *retp = hashTableIteratorFirst(&tableIterator);
    CHECK_RETV(retp, (char*)ex0);

    retv = hashTableIteratorIsLast(&tableIterator);
    CHECK_RETV(retv, 0);

    retp = hashTableIteratorNext(&tableIterator);
    CHECK_RETV(retp, (char*)ex8);

    retv = hashTableIteratorIsLast(&tableIterator);
    CHECK_RETV(retv, 0);

    retv = hashTableDelete(&table, (char*)ex11);
    CHECK_RETV(retv, 0);

    retv = hashTableDelete(&table, (char*)ex13);
    CHECK_RETV(retv, 0);

    retv = hashTableIteratorIsLast(&tableIterator);
    CHECK_RETV(retv, 1);

    return 1;
}

int testHashTableIteratorGet()
{
    char *retp = hashTableIteratorGet(NULL);
    CHECK_RETV_ERRNO(retp, NULL, EINVAL);

    hashTable_t table;
    int retv = hashTableCtor(&table, 16);
    CHECK_RETV(retv, 0);

    hashTableIterator_t tableIterator;
    retv = hashTableIteratorCtor(&tableIterator, &table);
    CHECK_RETV(retv, 0);

    /*for test purposes only*/
    tableIterator.hashTable = NULL;
    retp = hashTableIteratorGet(&tableIterator);
    CHECK_RETV_ERRNO(retp, NULL, EINVAL);

    /*restoring original value*/
    retv = hashTableIteratorCtor(&tableIterator, &table);
    CHECK_RETV(retv, 0);

    retp = hashTableIteratorGet(&tableIterator);
    CHECK_RETV_ERRNO(retp, NULL, EINVAL);

    char ex8[48] = "In law an infant, and in years a boy,";
    char ex0[48] = "In mind a slave to every vicious joy;";
    char ex13[48] = "From every sense of shame and virtue wean'd,";
    char ex11[48] = "In lies an adept, in deceit a fiend;";
    retv = hashTableInsert(&table, (char*)ex8);
    CHECK_RETV(retv, 8);
    retv = hashTableInsert(&table, (char*)ex0);
    CHECK_RETV(retv, 0);
    retv = hashTableInsert(&table, (char*)ex13);
    CHECK_RETV(retv, 13);
    retv = hashTableInsert(&table, (char*)ex11);
    CHECK_RETV(retv, 11);

    retp = hashTableIteratorFirst(&tableIterator);
    CHECK_RETV(retp, (char*)ex0);
    retp = hashTableIteratorGet(&tableIterator);
    CHECK_RETV(retp, (char*)ex0);

    retp = hashTableIteratorNext(&tableIterator);
    CHECK_RETV(retp, (char*)ex8);
    retp = hashTableIteratorGet(&tableIterator);
    CHECK_RETV(retp, (char*)ex8);

    retv = hashTableDelete(&table, (char*)ex11);
    CHECK_RETV(retv, 0);

    retp = hashTableIteratorNext(&tableIterator);
    CHECK_RETV(retp, (char*)ex13);
    retp = hashTableIteratorGet(&tableIterator);
    CHECK_RETV(retp, (char*)ex13);

    retv = hashTableDtor(&table);
    CHECK_RETV(retv, 0);
    return 1;
}

int main(int argc, char** argv)
{
    PRTEST(testHashTableCtor());
    PRTEST(testHashTableDtor());
    PRTEST(testHashTableInsert());
    PRTEST(testHashTableExpand());
    PRTEST(testHashTableFind());
    PRTEST(testHashTableDelete());
    PRTEST(testHashTableIteratorCtor());
    PRTEST(testHashTableIteratorFirst());
    PRTEST(testHashTableIteratorNext());
    PRTEST(testHashTableIteratorIsLast());
    PRTEST(testHashTableIteratorGet());

    return 0;
}