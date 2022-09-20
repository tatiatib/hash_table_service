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

#define INSERT 0
#define GET 1
#define DELETE 2
#define STOP 3

volatile int RUN;


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

void * process_requests(void * data){
    request * shared_request;
    memcpy(&shared_request, data, sizeof(void * ));
    hash_table * table;
    memcpy(&table, (char *)data + sizeof(void * ), sizeof(void * ));

    int cur_op = -1;
    char * key;
    size_t value_size;
    void * value;
    void * res;

    while (RUN){
        if(!sem_wait(&shared_request->sem)){

            cur_op = shared_request->op;
            if (cur_op == STOP){
                sem_post(&shared_request->sem);
                RUN = 0;
                break;
            }
            if (shared_request->read == 1){
                sem_post(&shared_request->sem);
                continue;
            }
            shared_request->read = 1;
            printf("cur operation %d executed by %ld\n", cur_op, pthread_self());
    
            key = parse_key(shared_request->buffer);
            
            if (cur_op == INSERT){
                value_size = parse_value_size(shared_request->buffer);
                value = parse_value(shared_request->buffer, value_size);
            }
            sem_post(&shared_request->sem);

            if (cur_op == INSERT){
                insert_key(table, key, value, value_size);
                res = get_key(table, key);
                printf("cur value is %d\n", *(int *)res);
                free(value);
                free(res);
                free(key);
            }else if(cur_op == GET){
                res = get_key(table, key);
                //SHOULD RETURN THIS
                free(res);
                free(key);
            }else if(cur_op == DELETE){
                delete_key(table, key);
                free(key);
            }

        }    
    }
    printf("THread is done .... \n");
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
    RUN = 1;

    request * shared_request = (request *)mmap(0, MEM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    sem_init(&shared_request->sem, 1, 0);

    void * data[2] = {(void *)shared_request, (void *)table};

    pthread_t thread_1, thread_2;

    pthread_create(&thread_1, NULL, process_requests, data); 
    pthread_create(&thread_2, NULL, process_requests, data); 

    pthread_join(thread_1, NULL);
    pthread_join(thread_2, NULL); 


    sem_close(&shared_request->sem);
    munmap(shared_request, MEM_SIZE);
    close(fd);
    shm_unlink(NAME);
    table_dispose(table);
    return 0;
}