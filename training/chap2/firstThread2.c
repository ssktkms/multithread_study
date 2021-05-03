#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void anotherFunc(int n) {
    if(n == 1) {
        printf("Hasta la vista, baby.\n");
        pthread_exit(NULL);
    }
}

void *threadFunc(void *arg) {
    int i;

    for(i = 0; i < 3; i++) {
        printf("I'm threadFunc: %d\n", i);
        anotherFunc(i);
        sleep(1);
    }
    return NULL;
}

int main(void) {
    pthread_t thread;
    int i;

    if(pthread_create(&thread, NULL, threadFunc, NULL) != 0) {
        printf("Error: Failed to create new thread.\n");
        exit(1);
    }

    for(i = 0; i < 5; i++) {
        printf("I'm main: %d\n", i);
        sleep(1);
    }

    return 0;
}
