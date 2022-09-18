#ifndef REQUEST_H
#define REQUEST_H
#include <semaphore.h>  

#define BUFFER_SIZE

typedef struct request{
    sem_t sem;
    int op;
    char buffer[BUFFER_SIZE];
}request;


#endif