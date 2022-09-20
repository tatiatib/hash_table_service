#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h> 
#include <string.h>
#include <unistd.h>
#include <time.h>


#define NAME "/shmem"
#define MEM_SIZE sizeof(request)

#define INSERT 0
#define GET 1
#define DELETE 2
#define STOP 3

#define KEY_POOL_SIZE 2
#define REQUESTS_NUM 1000000

void create_insert_operation(request * shared_request, char * key, int value){
    shared_request->op = INSERT;
    size_t key_length = strlen(key) + 1;
    size_t int_size = sizeof(int);
    memcpy((char * )shared_request->buffer, &key_length, sizeof(size_t));
    strcpy((char * )shared_request->buffer + sizeof(size_t), key);
    memcpy((char * )shared_request->buffer + sizeof(size_t)  + strlen(key) + 1, &int_size, sizeof(size_t));
    memcpy((char * )shared_request->buffer + sizeof(size_t) * 2  + strlen(key) + 1, &value, sizeof(int));
}

void create_delete_operation(request * shared_request, char * key){
    shared_request->op = DELETE;
    size_t key_length = strlen(key) + 1;
    size_t int_size = sizeof(int);
    memcpy((char * )shared_request->buffer, &key_length, sizeof(size_t));
    strcpy((char * )shared_request->buffer + sizeof(size_t), key);
}

void create_get_operation(request * shared_request, char * key){
    shared_request->op = GET;
    size_t key_length = strlen(key) + 1;
    size_t int_size = sizeof(int);
    memcpy((char * )shared_request->buffer, &key_length, sizeof(size_t));
    strcpy((char * )shared_request->buffer + sizeof(size_t), key);
    
}
/*
    Generate dummy key pool with random strings
*/
void generate_key_pool(char ** key_pool){
    char * alphabet = "abcdefghijklmnopqrstuvwxyz";
    int index;
    int cur_length;
    for (int i = 0; i < KEY_POOL_SIZE; i++){
        index = rand() % (int)(strlen(alphabet) - 1);
        cur_length = strlen(alphabet) - index;
        key_pool[i] = malloc(cur_length);
        strcpy(key_pool[i], alphabet + index);
    }
}

int main(){
    int fd = shm_open(NAME, O_RDWR, S_IRUSR | S_IWUSR);
    
    if (fd < 0){
        return 1;
    }

    request * shared_request = (request *)mmap(0, MEM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    clock_t start, end;
    start = clock();
    shared_request->read = 0;
    create_insert_operation(shared_request, "abcd", 234);
    sem_post(&shared_request->sem);
    char * key_pool[KEY_POOL_SIZE];

    generate_key_pool(key_pool);
    int i = 0;
    int rand_op;
    int rand_key;


    while (i < REQUESTS_NUM){
        if(!sem_wait(&shared_request->sem)){
            if (shared_request->read == 0){
                sem_post(&shared_request->sem);
                continue;
            }
            i += 1;
            shared_request->read = 0;
            rand_op = rand() % 3;
            rand_key = rand() % KEY_POOL_SIZE;
            if (rand_op == INSERT){
                create_insert_operation(shared_request, key_pool[rand_key], i);
            }else if(rand_op == GET){
                create_get_operation(shared_request, key_pool[rand_key]);
            }else if(rand_op == DELETE){
                create_delete_operation(shared_request, key_pool[rand_key]);
            }
            
            sem_post(&shared_request->sem);
        }
    }
    
    //Finish -  send operation stop
    while (1){
        if(!sem_wait(&shared_request->sem)){
            if (shared_request->read == 0){
                sem_post(&shared_request->sem);
                continue;
            }
            shared_request->read = 0;
            shared_request->op = STOP;
            sem_post(&shared_request->sem);
            break;
        }
    }
    
    end = clock();
    double time_elapsed = (double)(end - start)/CLOCKS_PER_SEC;
    printf("Time to process %d requests -  %f seconds.\n", REQUESTS_NUM, time_elapsed);
    printf("Thoughput %f requests/second\n", REQUESTS_NUM / time_elapsed);
    
    //Free up allocated memory
    munmap(shared_request, MEM_SIZE);
    close(fd);

    for (int i = 0; i < KEY_POOL_SIZE; i ++ ){
        free(key_pool[i]);
    }

    return  0;
}