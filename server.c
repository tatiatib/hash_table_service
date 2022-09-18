#include "hash_table.h"
#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h> 
#include <unistd.h>
#include <semaphore.h>  
#include <string.h>


#define NAME "/shmem"
#define MEM_SIZE sizeof(request)


char * parse_key(char * buffer){
    size_t length = *(size_t * )buffer;
    char * key = (char*) malloc(length);
    strcpy(key, buffer + sizeof(size_t));
    return key;
}

size_t parse_value_size(char * buffer){
    size_t length = *(size_t * )buffer;
    return *(size_t *)(buffer + sizeof(size_t) + length);
}

void * parse_value(char * buffer, size_t value_size){
    size_t length = *(size_t * )buffer;
    void * value = malloc(value_size);
    memcpy(value, buffer + sizeof(size_t)  * 2 + length, value_size);
    return value;
}

int main(int argc, char **argv){
    if (argc < 2){
        printf("Please specify capacity size");
        return 1;
    }
    int capacity = atoi(argv[1]);

    int fd = shm_open(NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    
    if (fd < 0){
        return 1;
    }

    ftruncate(fd, MEM_SIZE);
    hash_table * table = create_table(capacity);

    request * shared_request = (request *)mmap(0, MEM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    sem_init(&shared_request->sem, 1, 0); 
    int cur_op = -1;
    if(!sem_wait(&shared_request->sem)){
        cur_op = shared_request->op;
        char * key = parse_key(shared_request->buffer);
        size_t value_size = parse_value_size(shared_request->buffer);
        void * value = parse_value(shared_request->buffer, value_size);
        sem_post(&shared_request->sem);
        insert_key(table, key, value, value_size);
        void *  test_value = get_key(table, key);
        printf("key is %d\n", *(int *)value);
        free(key);
        free(value);
        
    }    

    sem_close(&shared_request->sem);
    munmap(shared_request, MEM_SIZE);
    close(fd);
    shm_unlink(NAME);
    table_dispose(table);
    return 0;
}