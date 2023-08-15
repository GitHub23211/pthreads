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
#define MAX_RESOLVER_THREADS 5

/* Declare global variables */
sem_t q_sem;
sem_t c_sem;
sem_t w_sem;
queue q;
int counter;

int main(int argc, char** argv) {

    /* Error checking before initialising variables*/
    int num_input = argc - 2;
    pthread_t requesters[MAX_REQUESTER_THREADS];
    pthread_t resolvers[MAX_RESOLVER_THREADS];
    FILE* input_files[num_input];
    queue_init(&q, QUEUESIZE);
    sem_init(&q_sem, 0, 1);
    sem_init(&c_sem, 0, 1);
    sem_init(&w_sem, 0, 1);

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
	    pthread_join(requesters[t],NULL);
    }
    for(int t=0;t<MAX_REQUESTER_THREADS;t++){
        pthread_join(resolvers[t],NULL);
    }
    printf("All of the threads were completed! Counter is: %d\n", counter);

    /* Clean up */
    queue_cleanup(&q);
    fclose(output);
    return 0;
}

void* request(void* input) {
    char buffer[SBUFSIZE];
    FILE* ifp = (FILE*)input;

    while(!feof(ifp)) {
        if(!queue_is_full(&q)) {
            fscanf(ifp, INPUTFS, buffer);
            tsafe_queue_push(&q, buffer);
            printf("%s\n", buffer);
        }
        else {
            usleep((rand()%100));
        }
    }
    fclose(ifp);
    return NULL;
}

void* resolve(void* output) {
    char* temp;
    FILE* ofp = (FILE*)output;
    char firstipstr[INET6_ADDRSTRLEN];
    int c = 0;

    while(!queue_is_empty(&q)) {
        temp = tsafe_queue_pop(&q);
        printf("contents: %s\n", temp);
        if(dnslookup(temp, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE) {
            strncpy(firstipstr, "", sizeof(firstipstr));
            fprintf(output, "%s\n", temp);
	    }
        else {
            tsafe_increment();
            tsafe_write(output, temp, firstipstr);
            fprintf(output, "%d.%s,%s\n", counter, temp, firstipstr);
        }
        free(temp);
        temp = NULL;
        c++;
    }
    printf("resolver thread ended! count of resolves: %d\n", c);
}

void tsafe_queue_push(queue* q, char* buffer) {
    sem_wait(&q_sem);
    queue_push(q, strcpy((char*)malloc(SBUFSIZE), buffer));
    sem_post(&q_sem);
}

char* tsafe_queue_pop(queue* q) {
    sem_wait(&q_sem);
    char* hostname = (char*)queue_pop(q);
    sem_post(&q_sem);
    return hostname;
}

void tsafe_increment() {
    sem_wait(&c_sem);
    counter++;
    sem_post(&c_sem);
}

void tsafe_write(FILE* output, char* hostname, char* ip) {
    sem_wait(&w_sem);
    fprintf(output, "%d.%s,%s\n", counter, hostname, ip);
    sem_post(&w_sem);
}