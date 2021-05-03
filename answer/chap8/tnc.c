/**
 * Tiny Net Cat
 * This code was designed and coded by SHIBUYA K.
 */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char *const argv[]) {
    if(argc != 3) {
        fprintf(stderr, "tnc server_name port_number\n");
        return 1;
    }
    const char *server = argv[1];
    int portno = atoi(argv[2]);

    // ソケット作成
    int s = socket(PF_INET, SOCK_STREAM, 0);
    if(s < 0) {
        perror("Can't create socket");
        return 1;
    }

    // サーバ名からIPアドレスを得る
    struct addrinfo hints, *ai;
    struct sockaddr_in addr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int err;
    if((err = getaddrinfo(server, NULL, &hints, &ai)) != 0) {
        fprintf(stderr, "Unknown host '%s' (%s)\n", server, gai_strerror(err));
        return 1;
    }
    if(ai->ai_addrlen > sizeof(addr)) {
        fprintf(stderr, "Invalid sockaddr length\n");
        freeaddrinfo(ai);
        return 1;
    }
    memcpy(&addr, ai->ai_addr, ai->ai_addrlen);
    addr.sin_port = htons(portno);
    freeaddrinfo(ai);

    // サーバに接続
    if(connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Can't connect");
        close(s);
        return 1;
    }

    // サーバソケットと標準入力をpoll
    struct pollfd pfd[2];
    char buf[4096];
    ssize_t len;
    while(1) {
        pfd[0].fd = s;
        pfd[0].events = POLLIN;
        pfd[1].fd = 0;
        pfd[1].events = POLLIN;
        if(poll(pfd, 2, -1) < 0) {
            perror("Poll error");
            close(s);
            return 1;
        }
        if(pfd[0].revents & POLLIN) {
            // サーバソケットから受信したものを標準出力へ
            len = read(s, buf, sizeof(buf));
            if(len <= 0)
                break;
            write(1, buf, (size_t)len);
        } else if(pfd[1].revents & POLLIN) {
            // 標準入力から受信したものをサーバソケットへ
            len = read(0, buf, sizeof(buf));
            if(len <= 0)
                break;
            write(s, buf, (size_t)len);
        } else if((pfd[0].revents & (POLLERR|POLLHUP|POLLNVAL)) || (pfd[1].revents & (POLLERR|POLLHUP|POLLNVAL))) {
            printf("socket error\n");
            break;
        }
    }

    close(s);

    return 0;
}
