#include <stdio.h>

#include "queue.h"

#define MIN_ARGS 2
#define MAX_ARGS 10
#define QUEUESIZE 10
#define MAX_RESOLVER_THREADS 10

/* Error codes */
#define MIN_ARG_ERR 1
#define MAX_ARG_ERR 2
#define INFILE_ERR 3
#define OUTFILE_ERR 4

void* request(void* input);
void* resolve(void* output);

int tsafe_queue_push(queue* q, void* p);
char* tsafe_queue_pop(queue* q);
int tsafe_queue_full(queue* q);
int tsafe_queue_empty(queue* q);

void tsafe_write(FILE* output, char* hostname, char* ip);
void tsafe_write_error(FILE* output, char* hostname, char* ip);

void tsafe_increment();
void tsafe_add_pusher();
void tsafe_decerement_pusher();