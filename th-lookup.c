#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "th-lookup.h"
#include "./sample/queue.h"
#include "./sample/util.h"

#define MINARGS 2
#define QUEUESIZE 10
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

int main(int argc, char** argv) {
    FILE* input = NULL;
    FILE* output = NULL;
    char buffer[SBUFSIZE];
    char firstipstr[INET6_ADDRSTRLEN];

    input = fopen(argv[1], "r");
    output = fopen(argv[2], "w");

    queue q;
    queue_init(&q, QUEUESIZE);
    while(fscanf(input, INPUTFS, buffer) > 0) {
        queue_push(&q, strcpy(((char*)malloc(SBUFSIZE)), buffer));
    }

    while(!queue_is_empty(&q)) {
        char* temp =  (char*)queue_pop(&q);
        printf("contents: %s\n", temp);
        if(dnslookup(temp, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE) {
            strncpy(firstipstr, "", sizeof(firstipstr));
	    }
        free(temp);
	    fprintf(output, "%s,%s\n", temp, firstipstr);
    }

    queue_cleanup(&q);
    fclose(input);
    fclose(output);

    return 0;
}