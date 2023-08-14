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
#define MAX_THREADS 4

sem_t q_sem;
sem_t c_sem;
sem_t w_sem;

int counter = 0;

int main(int argc, char** argv) {
    FILE* input = NULL;
    FILE* output = NULL;
    pthread_t requesters[MAX_THREADS];
    pthread_t resolvers[MAX_THREADS];
    req_arguments req_args;

    input = fopen(argv[1], "r");
    output = fopen(argv[2], "w");

    queue q;
    queue_init(&q, QUEUESIZE);
    sem_init(&q_sem, 0, 1);
    sem_init(&c_sem, 0, 1);
    sem_init(&w_sem, 0, 1);

    req_args.qp = &q;
    req_args.ifp = input;
    req_args.ofp = output;
    req_arguments* strp = &req_args;

    for(int i = 0; i < MAX_THREADS; i++) {
        int reqt_err = pthread_create(&(requesters[i]), NULL, request, (void*)strp);
        int rest_err = pthread_create(&(resolvers[i]), NULL, resolve, (void*)strp);
        printf("Creating threads: %d\n", i);
        if (reqt_err || rest_err){
            printf("ERROR; return code from pthread_create() is %d\n", reqt_err);
            printf("ERROR; return code from pthread_create() is %d\n", rest_err);
            exit(2);
	    }
    }

    for(int t=0;t<MAX_THREADS;t++){
	    pthread_join(requesters[t],NULL);
        pthread_join(resolvers[t],NULL);
    }
    printf("All of the threads were completed! Counter is: %d\n", counter);

    queue_cleanup(&q);
    fclose(input);
    fclose(output);

    return 0;
}

void* request(void* args) {
    char buffer[SBUFSIZE];
    req_arguments* rargs = args;
    FILE* input = rargs->ifp;
    queue* q = rargs->qp;

    while(!feof(input)) {
        if(!queue_is_full(q)) {
            fscanf(input, INPUTFS, buffer);
            tsafe_queue_push(q, buffer);
            printf("%s\n", buffer);
        }
        usleep((rand()%100));
    }
    return NULL;
}

void* resolve(void* args) {
    char* temp;
    req_arguments* rargs = args;
    FILE* output = rargs->ofp;
    queue* q = rargs->qp;
    char firstipstr[INET6_ADDRSTRLEN];

    while(!queue_is_empty(q)) {
        temp = tsafe_queue_pop(q);
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
    }
    temp = NULL;
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