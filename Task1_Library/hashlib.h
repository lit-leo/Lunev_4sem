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

typedef struct bitArray bitArray_t;

typedef struct hashTable
{
    unsigned int capacity;
    unsigned int used;
    bitArray_t *inSequence;
    char** table;
} hashTable_t;

/****
* Initialize the table.
* Adjusts capacity of the table to nearest power of 2.
*/
int hashTableCtor(hashTable_t *this, unsigned int size);

/****
* Destructor. Frees all memory.
*/
int hashTableDtor(hashTable_t *this);

/****
* Inserts string into the table.
*/
int hashTableInsert(hashTable_t *this, char* data);

/****
* Tries to find element in the table.
* Returns index to some entry with matching data
*/
int hashTableFind(hashTable_t *this, char *data);

/****
* Deletes element from table.
*/
int hashTableDelete(hashTable_t *this, char *data);

/*<<<< REMOVE AFTER TESTS >>>>*/
unsigned int hashRot13(const char * string);

unsigned int hashLY_odd(const char* string);
/*<<<<<<<<<<<<<<>>>>>>>>>>>>>>*/
#endif