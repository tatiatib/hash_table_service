#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h> 
#include <string.h>
#include <unistd.h>

#define NAME "/shmem"
#define MEM_SIZE sizeof(request)

void create_insert_operation(request * shared_request, char * key, int value){
    shared_request->op = 123;
    size_t key_length = strlen(key) + 1;
    size_t int_size = sizeof(int);
    memcpy((char * )shared_request->buffer, &key_length, sizeof(size_t));
    strcpy((char * )shared_request->buffer + sizeof(size_t), key);
    printf("this is the key %s\n", (char * )shared_request->buffer + sizeof(size_t));
    memcpy((char * )shared_request->buffer + sizeof(size_t)  + strlen(key) + 1, &int_size, sizeof(size_t));
    memcpy((char * )shared_request->buffer + sizeof(size_t) * 2  + strlen(key) + 1, &value, sizeof(int));
}

int main(){
    int fd = shm_open(NAME, O_RDWR, S_IRUSR | S_IWUSR);
    
    if (fd < 0){
        return 1;
    }
    request * shared_request = (request *)mmap(0, MEM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    create_insert_operation(shared_request, "abcd", 234);
    sem_post(&shared_request->sem);

    munmap(shared_request, MEM_SIZE);
    close(fd);

    return  0;
}