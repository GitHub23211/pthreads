#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>


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

    /* Error checking before initialising variables */
    /* Check for correct number of arguments */
    if(argc < MIN_ARGS) {
        fprintf(stderr, 
            "ERROR: Not enough arguments. Arguments: %d \nUSAGE: ./th-lookup <input-files> <output-file>\n\tTakes up to 10 input files, and 1 output file\n"
            ,argc - 1);
        exit(1);
    }
    else if(argc > MAX_ARGS) {
        fprintf(stderr, 
            "ERROR: Too many arguments. Arguments: %d \nUSAGE: ./th-lookup <input-files> <output-file>\n\tTakes up to 10 input files, and 1 output file\n"
            ,argc - 1);
        exit(1);
    }

    /* Check for valid output file */
    FILE* output = fopen(argv[argc-1], "w");
    if(!output) {
        fprintf(stderr, "ERROR: Cannot find or access output file\n");
        exit(1);
    }

    /* Initialisation*/   
    int num_input = argc - 2;

    pthread_t requesters[num_input];
    pthread_t resolvers[MAX_RESOLVER_THREADS];

    FILE* input_files[num_input];

    counter = 0;
    req_count = 0;

    sem_init(&q_sem, 0, 1);
    sem_init(&c_sem, 0, 1);
    sem_init(&w_sem, 0, 1);
    sem_init(&req_sem, 0, 1);

    queue_init(&q, QUEUESIZE);

    /* Initialise one requester thread per input file*/
    int thread_count = 0;
    for(int i = 1; i <= num_input; i++) {
        input_files[thread_count] = fopen(argv[i], "r");
        if(input_files[thread_count]) {
            int err = pthread_create(&(requesters[thread_count]), NULL, request, input_files[thread_count]);
            if (err){
                printf("ERROR; return code from pthread_create() is %d\n", err);
                exit(2);
            }
            thread_count++;
        }
        else {
             fprintf(stderr, "ERROR: Cannot find or access input file: %s. Continuing...\n", argv[i]);
        }
    }

    /* Check if any requester threads were created */
    if(thread_count > 0) {
        /* Initialise all resolver threads */
        for(int j = 0; j < MAX_RESOLVER_THREADS; j++) {
            int err = pthread_create(&(resolvers[j]), NULL, resolve, (void*)output);
            if (err){
                printf("ERROR; return code from pthread_create() is %d\n", err);
                exit(2);
            }
        }

        /* Join threads */
        for(int i = 0; i < thread_count; i++){
            pthread_join(requesters[i],NULL);
        }
        for(int j = 0; j < MAX_RESOLVER_THREADS; j++){
            pthread_join(resolvers[j],NULL);
        }
        printf("All threads completed! Counter is: %d\n", counter);
        fclose(output);
    }
    else {
        /* No requester threads were created. */
        fprintf(stderr, "ERROR: No valid input files. Program did not run. Shutting down\n");        
    }

    /* Clean up */
    queue_cleanup(&q);
    sem_destroy(&q_sem);
    sem_destroy(&c_sem);
    sem_destroy(&w_sem);
    sem_destroy(&req_sem);
    return 0;
}

/** User-defined Functions */

void* request(void* input) {
    char buffer[SBUFSIZE];
    FILE* ifp = (FILE*)input;

    tsafe_add_requester();
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
    tsafe_remove_requester();
    return NULL;
}

void* resolve(void* output) {
    char firstipstr[INET6_ADDRSTRLEN];
    char* temp;
    FILE* ofp = (FILE*)output;

    /* 
     * Checks if there are items in the queue, 
     * or there are requester threads still pushing to the queue
     */
    while((temp = tsafe_queue_pop(&q)) != NULL|| req_count > 0) {
        /* 
         * Might be the case that while requester threads still exist,
         * there are no items in the queue. This will return NULL for temp.
         * Need to check for this.
         */
        if(temp) {
            if(dnslookup(temp, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE) {
                fprintf(stderr, "dnslookup error: %s\n", temp);
                strncpy(firstipstr, "", sizeof(firstipstr));
                tsafe_write_error(ofp, temp, firstipstr);
            }
            else {
                tsafe_write(ofp, temp, firstipstr);
                tsafe_increment();
            }
            free(temp);  
        }
    }
    
    temp = NULL;
    return NULL;
}

/** Thread-safe functions */

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

void tsafe_add_requester() {
    sem_wait(&req_sem);
    req_count++;
    sem_post(&req_sem);
}

void tsafe_remove_requester() {
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