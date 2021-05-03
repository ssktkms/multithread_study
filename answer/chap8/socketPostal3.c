#include "postalNumber.h"
#include "intqueue.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>

#define PORTNO 25000  /* 待ち受けポート番号 */
#define SEARCH_SIZE 100 /* 検索結果の最大取得数 */
#define N_WORKER 4 /* ワーカースレッド数 */
#define N_QUE 2 /* 接続要求キューサイズ */

static void getString(FILE *fp, char *buf, size_t buflen) {
    int ch;
    buflen--;
    while((ch = fgetc(fp)) != EOF) {
        if(ch == '\r')
            continue;
        if(ch == '\n')
            break;
        if(buflen > 0) {
            *(buf++) = ch;
            buflen--;
        }
    }
    *buf = '\0';
}

static void searchPostalNumber(FILE *fp) {
    fprintf(fp, "Search ? ");
    fflush(fp);
    char buf[128];
    getString(fp, buf, sizeof(buf));
    fprintf(fp, "Search for '%s':\n", buf);
    PostalNumber res[SEARCH_SIZE];
    size_t n = PostalNumberSearch(buf, res, SEARCH_SIZE);
    for(size_t i = 0; i < n; i++) {
        fprintf(fp, "  %s %s %s %s\n", res[i].code, res[i].pref, res[i].city, res[i].town);
    }
}

/* 全ワーカー共通のキュー */
static IntQueue *socQue;

/* ワーカスレッド処理 */
static void *doWorker(void *arg) {
    int id = (int)arg;
    printf("Start worker#%d\n", id);
    while(1) {
        /* クライアントソケットがキューに入るのを待つ */
        int soc;
        if(!IntQueueWait(socQue, 1000))
            continue; // タイムアウトしたのでリトライ
        if(!IntQueueGet(socQue, &soc))
            continue; // クライアントソケットは無かった

        /* FILEストリームとして取り扱えるようにsocをFILE*に変換 */
        FILE *fp;
        if((fp = fdopen(soc, "r+")) == NULL) {
            printf("Failed to create FILE stream\n");
            close(soc);
            break;
        }

        /* クライアントを端末として検索を実行する */
        searchPostalNumber(fp);

        fclose(fp); /* socもcloseされる */
    }
    printf("Finish worker#%d\n", id);

    return NULL;
}

int main(void) {
    PostalNumberLoadDB();

    /* ワーカースレッドの構築 */
    pthread_t worker[N_WORKER];
    for(int i = 0; i < N_WORKER; i++) {
        if(pthread_create(&worker[i], NULL, doWorker, (void *)i) != 0) {
            printf("Failed to create thread, abort.\n");
            return 1;
        }
    }
    /* ソケットのキューを作成 */
    if((socQue = IntQueueCreate(N_QUE)) == NULL) {
        printf("Failed to create IntQueue, abort.\n");
        return 1;
    }

    /* リクエストリスナーをオープンする */
    int listener;
    if((listener = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Can't create listener socket.\n");
        return 1;
    }
    /* ソケットが切れた時にサーバプロセスが落ちないようにするおまじない */
    signal(SIGPIPE, SIG_IGN);
    /* Address already in useを避けるおまじない */
    int val = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    /* ポートに割り当てて受信可能にする */
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(PORTNO);
    if(bind(listener, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        printf("Can't bind socket to port#%d\n", PORTNO);
        close(listener);
        return 1;
    }
    if(listen(listener, 1) < 0) {
        printf("Failed to listen on port#%d\n", PORTNO);
        close(listener);
        return 1;
    }
    printf("Waiting for connection on port#%d\n", PORTNO);

    while(1) {
        int soc;
        struct sockaddr_in caddr;
        socklen_t caddrlen = sizeof(caddr);
        /* acceptはクライアントからの接続があるまで待つ */
        if((soc = accept(listener, (struct sockaddr *)&caddr, &caddrlen)) < 0) {
            printf("Error on accept listning socket\n");
            break;
        }
        char ipstr[INET_ADDRSTRLEN];
        printf("Connect from %s\n", inet_ntop(AF_INET, &caddr.sin_addr, ipstr, sizeof(ipstr)));
        /* キューにソケットを入れておけばどれかのワーカーが処理してくれる */
        if(!IntQueueAdd(socQue, soc)) {
            printf("Socket que overflow.\n");
            close(soc);
        }
    }

    close(listener);

    /* ワーカスレッドの廃棄 */
    for(int i = 0; i < N_WORKER; i++) {
        pthread_join(worker[i], NULL);
    }

    /* キューの廃棄 */
    IntQueueDestroy(socQue);

    return 0;
}
