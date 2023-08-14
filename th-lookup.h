#include <stdio.h>

#include "./sample/queue.h"
typedef struct requester_arguements {
    queue* qp;
    FILE* ifp;
    FILE* ofp;
} req_arguments;

void* request(void* args);
void tsafe_queue_push(queue* q, char* buffer);
void* resolve(void* args);
char* tsafe_queue_pop(queue* q);
void tsafe_write(FILE* output, char* hostname, char* ip);
void tsafe_increment();