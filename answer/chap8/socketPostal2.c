#include "postalNumber.h"
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

/* ワーカースレッドごとのデータを保持する構造体 */
typedef struct {
    int id; /* ワーカスレッド番号（デバッグ用）*/
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int soc; /* 処理中のクライアントソケット */
    int busy; /* 処理中フラグ */
} WorkerContext;

/* ワーカスレッド処理 */
static void *doWorker(void *arg) {
    WorkerContext *worker = (WorkerContext *)arg;
    printf("Start worker#%d\n", worker->id);
    while(1) {
        /* busyフラグをOFFにして処理要求待ち */
        pthread_mutex_lock(&worker->mutex);
        worker->busy = 0;
        pthread_cond_wait(&worker->cond, &worker->mutex);
        /* 処理要求された */
        worker->busy = 1; /* 念のため */
        /* busyフラグが1の間はsocが書き換わることがないので、ここでロックを外してもよい */
        pthread_mutex_unlock(&worker->mutex);

        /* FILEストリームとして取り扱えるようにsocをFILE*に変換 */
        FILE *fp;
        if((fp = fdopen(worker->soc, "r+")) == NULL) {
            printf("Failed to create FILE stream\n");
            close(worker->soc);
            break;
        }

        /* クライアントを端末として検索を実行する */
        searchPostalNumber(fp);

        fclose(fp); /* worker->socもcloseされる */
    }
    printf("Finish worker#%d\n", worker->id);

    return NULL;
}

int main(void) {
    PostalNumberLoadDB();

    /* ワーカースレッドの構築 */
    WorkerContext worker[N_WORKER];
    for(int i = 0; i < N_WORKER; i++) {
        WorkerContext *w = &worker[i];
        w->id = i;
        pthread_mutex_init(&w->mutex, NULL);
        pthread_cond_init(&w->cond, NULL);
        w->soc = -1;
        w->busy = 1;
        if(pthread_create(&w->thread, NULL, doWorker, (void *)w) != 0) {
            printf("Failed to create thread, abort.\n");
            return 1;
        }
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
        /* 空きワーカーを探す */
        int i = 0;
        while(i < N_WORKER) {
            WorkerContext *w = &worker[i];
            /* busyフラグの読み書きを排他するためにロックする */
            pthread_mutex_lock(&w->mutex);
            if(!w->busy) {
                w->soc = soc;
                w->busy = 1;
                /* ワーカスレッドに通知 */
                pthread_cond_signal(&w->cond);
                pthread_mutex_unlock(&w->mutex);
                break;
            }
            pthread_mutex_unlock(&w->mutex);
            i++;
        }
        if(i >= N_WORKER) {
            /* 空きワーカが見つからなかった */
            printf("No more workers.\n");
            close(soc);
        }
    }

    close(listener);

    /* ワーカスレッドの廃棄 */
    for(int i = 0; i < N_WORKER; i++) {
        WorkerContext *w = &worker[i];
        pthread_join(w->thread, NULL);
        pthread_mutex_destroy(&w->mutex);
        pthread_cond_destroy(&w->cond);
    }

    return 0;
}
