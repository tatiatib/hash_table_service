
#ifndef HASH_TABLE_H
#define HASH_TABLE_H
#include <pthread.h>

typedef struct node{
    char * key;
    void * value;
    size_t value_size;
    struct node * next;
}node;

typedef struct hash_table{
    int cap;
    node ** values;
    pthread_mutex_t ** locks;

}hash_table;    

hash_table * create_table(int cap);
void table_dispose(hash_table * table);
void insert_key(hash_table * table, char * key, void * value, size_t value_size);
void * get_key(hash_table * table, char * key);
void delete_key(hash_table * table, char * key);


#endif