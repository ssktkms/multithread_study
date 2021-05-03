
static int stopRequest = 0;

void *threadFunc(void *arg) {
    .....
    char *buf = (char *)malloc(.....);
    .....
    while(stopRequest == 0) {
        // do something using buf.
        .....
    }
    free(buf);
    return NULL;
}

.....

int main(void) {
    .....
    if(pthread_create(&thread, NULL, threadFunc, NULL) != 0) {
        printf("Error: Failed to create new thread.\n");
        exit(1);
    }
    
    .....
    if( user wants to stop processing ) {
        stopRequest = 1;
        if(pthread_join(thread, NULL) != 0) {
            printf("Error: Failed to wait for the thread termination.\n");
            exit(1);
        }
    }
    .....
}
