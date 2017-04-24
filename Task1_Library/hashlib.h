/**********************************************
* FILENAME: hashlib.h
*
* DESCRIPTION:
*       This library represents yet another
*       implementation of a hash table that 
*       uses open adress collision resolution
*       method.
*
*       In order to keep performance rate high,
*       we double the size when it meets 
*       the threshold of 75%, so we can ensure 
*       that averagely it takes less than 4 
*       probes to insert a new string.
*
*       Table size also has to be a power of 2.      
*       
* AUTHOR: Litovchenko Leonid
*
* START DATE: 17 APR 2017
*
*/

/*использовать автоматическое увеличение размера таблицы
для поддержания высокого быстродействия*/

/*использовать битовый массив для индикации, принадлежит
ли ячейчка какой-нибудь из хеш последовательностей
По этому же битовому массиву проверять заполняемость таблицы.
*/

/*функция конструктор принимает любое значение, но т.к.
размер должен быть степенью 2, то округляет размер до
степени 2 в большую сторону.
*/

#ifndef HASHLIB_H
#define HASHLIB_H
#define DBG_HASH_FUNCTIONS

typedef struct bitArray bitArray_t;

typedef struct hashTable
{
    unsigned int capacity;
    unsigned int used;
    bitArray_t *inSequence;
    char** table;
} hashTable_t;

typedef struct hashTableIterator hashTableIterator_t; 

/****
* Initializes hashTbable structure on pointer given.
* Adjusts capacity of the table to nearest power of 2.
* Returns 0 on success.
* If an error occurs, sets ernno appropriately and returns -1.
*/
int hashTableCtor(hashTable_t *this, unsigned int size);

/****
* Frees memory allocated in hashTableCtor and 
* poisons fields of the structure.
* Returns 0 on success.
* If invalid argument given, sets errno to EINVAL and returns -1.
*/
int hashTableDtor(hashTable_t *this);

/****
* Inserts string "data" into the table pointed by "this".
* If load factor exceeds the threshold of 0.75, initiates
* expansion of the table to double its capacity.
* Returns 0 on success.
* If an insertion failed, returns -1;
* If an error occurs, sets ernno appropriately and returns -2.
*/
int hashTableInsert(hashTable_t *this, char* data);

/****
* Tries to find element in the table.
* Returns index of the matching entry on succes
* If an error occurs, sets ernno appropriately and returns -1.
*/
int hashTableFind(hashTable_t *this, char *data);

/****
* Deletes element from table.
* Returns 0 on success.
* If deleting fails due to the lack of the matching entry
* returns -1.
* If an error occurs, sets ernno appropriately and returns -2.
*/
int hashTableDelete(hashTable_t *this, char *data);

/****
* Creates new hashTableIterator from the given hashTable
*/
int hashTableIteratorCtor(hashTableIterator_t *this, hashTable_t *from);

/****
* Returns first filled entry of the hash table.
*/
char *hashTableIteratorFirst(hashTableIterator_t *this);

/****
* Returns next filled entry of the hash table.
*/
char *hashTableIteratorNext(hashTableIterator_t *this);

/****
* Check if last filled entry of the table met.
*/
int hashTableIteratorIsLast(hashTableIterator_t *this);

/****
* Returns current string.
*/
char *hashTableIteratorGet(hashTableIterator_t *this);
#endif