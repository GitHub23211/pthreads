#include <stdio.h>

#include "queue.h"

#define MIN_ARGS 3
#define MAX_ARGS 12
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define QUEUESIZE 100
#define MAX_RESOLVER_THREADS 10

void* request(void* input);
void* resolve(void* output);

int tsafe_queue_push(queue* q, void* p);
char* tsafe_queue_pop(queue* q);
int tsafe_queue_full(queue* q);
int tsafe_queue_empty(queue* q);

void tsafe_write(FILE* output, char* hostname, char* ip);
void tsafe_write_error(FILE* output, char* hostname, char* ip);

void tsafe_increment();
void tsafe_add_requester();
void tsafe_remove_requester();
