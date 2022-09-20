#include "hash_table.h"
#include <stdlib.h> 
#include <string.h>
#include <stdio.h>

/*
djb2
Simple hash function for testing porpuses from http://www.cse.yorku.ca/~oz/hash.html
*/
unsigned long hash(char *str){
    unsigned long hash = 5381;
    int c;

    while (c = (*str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

hash_table * create_table(int cap){
    hash_table * table = (hash_table *) malloc(sizeof(hash_table));
    table->cap = cap;
    table->values = (node ** )calloc(cap, sizeof(node *));
    table->locks = (pthread_mutex_t ** ) calloc(cap, sizeof(pthread_mutex_t * ));
    for (int i = 0; i < cap; i ++ ){
        node * head = (node *)malloc(sizeof(node));
        head->key = NULL;
        head->value = NULL;
        head->next = NULL;
        table->values[i] = head;
        table->locks[i] = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(table->locks[i], NULL);
    }

    return table;
}


void free_nodes(node * head){
    node * cur = head;
    while (cur != NULL){
        node * next = cur->next;
        free(cur);
        cur = next;
    }
}

void table_dispose(hash_table * table){
    for (int i = 0; i < table->cap; i ++ ){
        free_nodes(table->values[i]);
        pthread_mutex_destroy(table->locks[i]);
    }
    free(table);
}

void change_value(node * cur, void * value, size_t value_size){
    free(cur->value);
    cur->value = malloc(value_size);
    memcpy(cur->value, value, value_size);
    cur->value_size = value_size;
}

void insert_key(hash_table *table, char * key, void * value, size_t value_size){
    unsigned long hash_value = hash(key);
    int index = (int)(hash_value % table->cap);
    pthread_mutex_lock(table->locks[index]);
    node * cur = table->values[index];
    node * prev = NULL;
    while (cur != NULL){
        if (cur->key != NULL && (strcmp(cur->key, key) == 0)){
            change_value(cur, value, value_size);
            pthread_mutex_unlock(table->locks[index]);
            return;
        }
        prev = cur;
        cur = cur->next;
    }

    cur = prev;
    node * new_node = (node *) malloc(sizeof(node));
    new_node->key = (char*) malloc(strlen(key) + 1);
    strcpy(new_node->key, key);
    new_node->value = malloc(value_size);
    memcpy(new_node->value, value, value_size);
    new_node->value_size = value_size;
    new_node->next = NULL;
    cur->next = new_node;
    pthread_mutex_unlock(table->locks[index]);
}

void * get_key(hash_table * table, char * key){
    unsigned long hash_value = hash(key);
    int index = (int)(hash_value % table->cap);
    pthread_mutex_lock(table->locks[index]);
    node * cur = table->values[index]->next;
    while (cur != NULL){
        if (strcmp(cur->key, key) == 0) break;
        cur = cur->next;
    }

    void * res;
    if (cur == NULL){
        res = NULL;
    }else{
        res = malloc(cur->value_size);
        memcpy(res, cur->value, cur->value_size);
    }
    
    pthread_mutex_unlock(table->locks[index]);
    return res;
}

void delete_node(node * prev, node * cur){
    prev->next = cur->next;
    free(cur->value);
    free(cur);
}


void delete_key(hash_table * table, char * key){
    unsigned long hash_value = hash(key);
    int index = (int)(hash_value % table->cap);
    pthread_mutex_lock(table->locks[index]);
    node * prev = table->values[index];
    node * cur = table->values[index]->next;
    while (cur != NULL){
        if (strcmp(cur->key, key) == 0){
            delete_node(prev, cur);
            break;
        }
        prev = cur;
        cur = cur->next;
    }
    pthread_mutex_unlock(table->locks[index]);
}   