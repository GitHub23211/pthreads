#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

#include "th-lookup.h"
#include "./sample/queue.h"
#include "./sample/util.h"

#define MINARGS 2
#define QUEUESIZE 10
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define MAX_REQUESTER_THREADS 2
#define MAX_RESOLVER_THREADS 10

/* Declare global variables */
sem_t q_sem;
sem_t c_sem;
sem_t push_sem;
sem_t w_sem;

queue q;
int counter;
int pushers;

int main(int argc, char** argv) {

    /* Error checking before initialising variables*/
    int num_input = argc - 2;
    pthread_t requesters[num_input];
    pthread_t resolvers[MAX_RESOLVER_THREADS];
    FILE* input_files[num_input];
    queue_init(&q, QUEUESIZE);
    sem_init(&q_sem, 0, 1);
    sem_init(&c_sem, 0, 1);
    sem_init(&push_sem, 0, 1);
    sem_init(&w_sem, 0, 1);
    counter = 0;
    pushers = 0;

    FILE* output = fopen(argv[argc-1], "w");

    /* Initialise one requester thread per input file*/
    for(int i = 0; i < num_input; i++) {
        input_files[i] = fopen(argv[i+1], "r");
        int err = pthread_create(&(requesters[i]), NULL, request, input_files[i]);
        printf("Creating requester threads: %d\n", i);
        if (err){
            printf("ERROR; return code from pthread_create() is %d\n", err);
            exit(2);
	    }
    }

    /* Initialise all resolver threads */
    for(int j = 0; j < MAX_RESOLVER_THREADS; j++) {
        printf("Creating resolver threads: %d\n", j);
        int err = pthread_create(&(resolvers[j]), NULL, resolve, (void*)output);
        if (err){
            printf("ERROR; return code from pthread_create() is %d\n", err);
            exit(2);
	    }
    }

    /* Gather threads */
    for(int t=0;t<num_input;t++){
	    int res = pthread_join(requesters[t],NULL);
    }
    for(int t=0;t<MAX_RESOLVER_THREADS;t++){
        int res = pthread_join(resolvers[t],NULL);
    }
    printf("All of the threads were completed! Counter is: %d\n", counter);
    printf("is queue empty? %d\n", queue_is_empty(&q));

    /* Clean up */
    queue_cleanup(&q);
    fclose(output);
    sem_destroy(&q_sem);
    sem_destroy(&c_sem);
    sem_destroy(&w_sem);
    sem_destroy(&push_sem);
    return 0;
}

void* request(void* input) {
    char buffer[SBUFSIZE];
    FILE* ifp = (FILE*)input;
    int c = 0;

    tsafe_add_pusher();
    while(!feof(ifp)) {
        if(!tsafe_queue_full(&q)) {
            int res = fscanf(ifp, INPUTFS, buffer);
            if(res >= 0) {
                c++;
                tsafe_queue_push(&q, strcpy((char*)malloc(SBUFSIZE), buffer));
            }
        }
        else {
            usleep((rand()%100));
        }
    }
    fclose(ifp);
    tsafe_decerement_pusher();
    printf("requester done, request count: %d!\n", c);
    return NULL;
}

void* resolve(void* output) {
    char* temp;
    FILE* ofp = (FILE*)output;
    char firstipstr[INET6_ADDRSTRLEN];

    while(!tsafe_queue_empty(&q) || pushers > 0) {
        temp = tsafe_queue_pop(&q);
        if(temp != NULL) {
            //printf("resolver temp: %s\n", temp);
            if(dnslookup(temp, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE) {
                fprintf(stderr, "dnslookup error: %s\n", temp);
                strncpy(firstipstr, "", sizeof(firstipstr));
                tsafe_write(output, temp, firstipstr);
            }
            else {
                tsafe_write(output, temp, firstipstr);
                tsafe_increment();
            }
            free(temp);
        }
    }
    temp = NULL;
    return NULL;
}

void tsafe_queue_push(queue* q, void* p) {
    sem_wait(&q_sem);
    queue_push(q, p);
    sem_post(&q_sem);
}

char* tsafe_queue_pop(queue* q) {
    sem_wait(&q_sem);
    char* hostname = (char*)queue_pop(q);
    sem_post(&q_sem);
    return hostname;
}

int tsafe_queue_full(queue* q) {
    int bool = 0;
    sem_wait(&q_sem);
    bool = queue_is_full(q);
    sem_post(&q_sem);
    return bool;
}

int tsafe_queue_empty(queue* q) {
    int bool = 0;
    sem_wait(&q_sem);
    bool = queue_is_empty(q);
    sem_post(&q_sem);
    return bool;
}

void tsafe_increment() {
    sem_wait(&c_sem);
    counter++;
    sem_post(&c_sem);
}

void tsafe_add_pusher() {
    sem_wait(&push_sem);
    pushers++;
    sem_post(&push_sem);
}

void tsafe_decerement_pusher() {
    sem_wait(&push_sem);
    pushers--;
    sem_post(&push_sem);
}

void tsafe_write(FILE* output, char* hostname, char* ip) {
    sem_wait(&w_sem);
    fprintf(output, "%d. %s,%s\n", counter, hostname, ip);
    sem_post(&w_sem);
}