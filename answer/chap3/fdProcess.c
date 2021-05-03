#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

void processFunc() {
    const char *msg = "processFunc writes to standard output\n";
    write(1, msg, strlen(msg));
    close(1);
    open("fdProcess.txt", O_WRONLY|O_CREAT|O_TRUNC, 0744);
    msg = "processFunc writes to fdProcess.txt\n";
    write(1, msg, strlen(msg));
    sleep(2);
    exit(0);
}

int main(void) {
    pid_t process;

    if((process = fork()) == 0) {
        // this is child process
        processFunc();
    }

    // this is main process
    sleep(1);

    const char *msg = "main writes to standard output\n";
    write(1, msg, strlen(msg));

    waitpid(process, NULL, 0);

    return 0;
}
