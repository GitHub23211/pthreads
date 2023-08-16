#include <stdio.h>

#include "./sample/queue.h"

void* request(void* input);
void* resolve(void* output);
void tsafe_queue_push(queue* q, void* p);
char* tsafe_queue_pop(queue* q);
void tsafe_write(FILE* output, char* hostname, char* ip);
void tsafe_increment();
void tsafe_add_pusher();
void tsafe_decerement_pusher();
int tsafe_queue_full(queue* q);
int tsafe_queue_empty(queue* q);