#ifndef REQUEST_H
#define REQUEST_H
#include <semaphore.h>  

#define BUFFER_SIZE

typedef struct request{
    sem_t sem;
    int op;
    int read;
    char buffer[BUFFER_SIZE];
}request;


#endif