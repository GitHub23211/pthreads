#include <stdio.h>

#include "./sample/queue.h"

void* request(void* input);
void* resolve(void* output);
void tsafe_queue_push(queue* q, char* buffer);
char* tsafe_queue_pop(queue* q);
void tsafe_write(FILE* output, char* hostname, char* ip);
void tsafe_increment();