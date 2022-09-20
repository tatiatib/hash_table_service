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
#define N_THREADS 8

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
                RUN = 0;  //no need to lock  here - only on thread read op -stop 
                break;
            }
            if (shared_request->read == 1){
                sem_post(&shared_request->sem);
                continue;
            }
            shared_request->read = 1;
            key = parse_key(shared_request->buffer);
            
            if (cur_op == INSERT){
                value_size = parse_value_size(shared_request->buffer);
                value = parse_value(shared_request->buffer, value_size);
            }
            sem_post(&shared_request->sem); //once data is parsed and operation is read - give access to client
            //process operation
            if (cur_op == INSERT){
                insert_key(table, key, value, value_size);
                res = get_key(table, key);
                free(value);
                free(res);
                free(key);
            }else if(cur_op == GET){
                res = get_key(table, key);
                free(res);
                free(key);
            }else if(cur_op == DELETE){
                delete_key(table, key);
                free(key);
            }

        }    
    }
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
    printf("Running %d threads\n", N_THREADS);
    //Share  shared memory to threads
    void * data[2] = {(void *)shared_request, (void *)table};

    pthread_t threads[N_THREADS];
    for (int i = 0; i < N_THREADS; i ++ ){
        if (pthread_create(&threads[i], NULL, process_requests, data) != 0){
          break;
        }
    }

    for (int i = 0; i < N_THREADS; i ++ ){
        pthread_join(threads[i], NULL);
    }
    
    printf("All threads done\n");
    sem_close(&shared_request->sem);
    munmap(shared_request, MEM_SIZE);
    close(fd);
    shm_unlink(NAME);
    table_dispose(table);
    return 0;
}