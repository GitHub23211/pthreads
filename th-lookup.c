#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

#include "th-lookup.h"
#include "util.h"
#include "queue.h"

/* Declare global variables */
sem_t q_sem;
sem_t c_sem;
sem_t w_sem;
sem_t req_sem;

queue q;
int counter;
int req_count;

int main(int argc, char** argv) {

    /* Error checking before initialising variables
    if(argc < MIN_ARGS) {
        print_error(MIN_ARG_ERR);
    }
    else if(argc > MAX_ARGS) {
        print_error(MAX_ARG_ERR);
    }*/

    /* Variable initialisation*/
    int num_input = argc - 2;
    pthread_t requesters[num_input];
    pthread_t resolvers[MAX_RESOLVER_THREADS];
    FILE* input_files[num_input];
    queue_init(&q, QUEUESIZE);
    sem_init(&q_sem, 0, 1);
    sem_init(&c_sem, 0, 1);
    sem_init(&w_sem, 0, 1);
    sem_init(&req_sem, 0, 1);
    counter = 0;
    req_count = 0;
    FILE* output = fopen(argv[argc-1], "w");

    /* Initialise one requester thread per input file*/
    for(int i = 0; i < num_input; i++) {
        input_files[i] = fopen(argv[i+1], "r");
        printf("Creating requester threads: %d\n", i);
        int err = pthread_create(&(requesters[i]), NULL, request, input_files[i]);
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

    /* Join threads */
    for(int t=0;t<num_input;t++){
	    int res = pthread_join(requesters[t],NULL);
    }
    for(int t=0;t<MAX_RESOLVER_THREADS;t++){
        int res = pthread_join(resolvers[t],NULL);
    }

    printf("All of the threads were completed! Counter is: %d\n", counter);

    /* Clean up */
    queue_cleanup(&q);
    fclose(output);
    sem_destroy(&q_sem);
    sem_destroy(&c_sem);
    sem_destroy(&w_sem);
    sem_destroy(&req_sem);
    return 0;
}

void* request(void* input) {
    char buffer[SBUFSIZE];
    FILE* ifp = (FILE*)input;

    tsafe_add_pusher();
    while(!feof(ifp)) {
        int res = fscanf(ifp, INPUTFS, buffer);
        if(res >= 0) {
            char* temp = (char*)malloc(SBUFSIZE);
            while(tsafe_queue_push(&q, strcpy(temp, buffer)) == QUEUE_FAILURE) {
                usleep((rand()%100));
            }
        }
    }
    fclose(ifp);
    tsafe_decerement_pusher();
    return NULL;
}

void* resolve(void* output) {
    char* temp;
    FILE* ofp = (FILE*)output;
    char firstipstr[INET6_ADDRSTRLEN];

    while((temp = tsafe_queue_pop(&q)) != NULL|| req_count > 0) {
        if(temp) {
            if(dnslookup(temp, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE) {
                fprintf(stderr, "dnslookup error: %s\n", temp);
                strncpy(firstipstr, "", sizeof(firstipstr));
                tsafe_write_error(output, temp, firstipstr);
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

int tsafe_queue_push(queue* q, void* p) {
    sem_wait(&q_sem);
    int res = queue_push(q, p);
    sem_post(&q_sem);
    return res;
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

void tsafe_add_pusher() {
    sem_wait(&req_sem);
    req_count++;
    sem_post(&req_sem);
}

void tsafe_decerement_pusher() {
    sem_wait(&req_sem);
    req_count--;
    sem_post(&req_sem);
}

void tsafe_write(FILE* output, char* hostname, char* ip) {
    sem_wait(&w_sem);
    fprintf(output, "%d. %s,%s\n", counter, hostname, ip);
    sem_post(&w_sem);
}

void tsafe_write_error(FILE* output, char* hostname, char* ip) {
    sem_wait(&w_sem);
    fprintf(output, "%s,%s\n", hostname, ip);
    sem_post(&w_sem);
}
/*
void print_error(int err) {
    switch(err) {
        case MIN_ARG_ERR:
        //
        exit(1)
        break;
        case MAX_ARG_ERR:
        //
        exit(1);
        break;
        case INFILE_ERR:
        //
        break;
        case OUTFILE_ERR:
        //
        break;
        default:
        //
    }
}
*/
