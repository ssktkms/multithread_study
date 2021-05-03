#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define SIZE 10000000

void *threadFunc(void *arg) {
    double table[SIZE];
    int i;

    for(i = 0; i < SIZE; i++) {
        table[i] = i*3.14;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_attr_t attr;
    pthread_t thread;

    pthread_attr_init(&attr);
    if(pthread_attr_setstacksize(&attr, SIZE*sizeof(double)+100000) != 0) {
        printf("Failed to set stack size.\n");
        exit(1);
    }

    if(pthread_create(&thread, &attr, threadFunc, NULL) != 0) {
        printf("Error: Failed to create new thread.\n");
        exit(1);
    }

    if(pthread_join(thread, NULL) != 0) {
        printf("Error: Failed to wait for the thread termination.\n");
        exit(1);
    }

    return 0;
}
