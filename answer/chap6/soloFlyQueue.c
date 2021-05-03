#include "xyqueue.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <errno.h>


#define WIDTH 78   /* スクリーン幅 */
#define HEIGHT 23  /* スクリーン高さ */
#define MAX_FLY 1  /* 描画するハエの数 */

#define QUEUE_SIZE 10 /* 目標地点キュー要素数 */

static int stopRequest;  /* スレッド終了フラグ */
static int drawRequest;  /* 描画要求フラグ */
static pthread_mutex_t drawMutex; /* 描画要求待ち用ミューテックス */
static pthread_cond_t  drawCond;  /* 描画要求待ち用ミューテックス */

static void requestDraw(void);

/*
 * ミリ秒単位でスリープする
 */
void mSleep(int msec) {
    struct timespec ts;
    ts.tv_sec = msec/1000;
    ts.tv_nsec = (msec%1000)*1000000;
    nanosleep(&ts, NULL);
}

/*
 * ミリ秒単位で条件待ちをする
 */
int pthread_cond_timedwait_msec(pthread_cond_t *cond, pthread_mutex_t *mutex, long msec) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += msec/1000;
    ts.tv_nsec += (msec%1000)*1000000;
    if(ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }
    return pthread_cond_timedwait(cond, mutex, &ts);
}

/*
 * 画面クリア
 */
void clearScreen() {
    fputs("\033[2J", stdout); /* このエスケープコードをターミナルに送ると画面がクリアされる */
}

/*
 * カーソル移動
 */
void moveCursor(int x, int y) {
    printf("\033[%d;%dH", y, x); /* このエスケープコードをターミナルに送るとカーソル位置がx,yになる。*/
}

/*
 * カーソル位置保存
 */
void saveCursor() {
    printf("\0337"); /* このエスケープコードをターミナルに送るとカーソル位置を記憶する */
}

/*
 * カーソル位置復帰
 */
void restoreCursor() {
    printf("\0338"); /* このエスケープコードをターミナルに送ると記憶したカーソル位置に戻る */
}

/*
 * ハエ構造体
 */
typedef struct {
    char mark;    /* 表示キャラクタ */
    double x, y;  /* 座標 */
    double angle; /* 移動方向（角度）*/
    double speed; /* 移動速度（ピクセル/秒）*/
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    XYQueue *destQue; /* 目標地点キュー */
} Fly;

Fly flyList[MAX_FLY];

/*
 * ハエの状態を初期化
 */
void FlyInitCenter(Fly *fly, char mark_) {
    fly->mark = mark_;
    pthread_mutex_init(&fly->mutex, NULL);
    pthread_cond_init(&fly->cond, NULL);
    fly->x = (double)WIDTH/2.0;
    fly->y = (double)HEIGHT/2.0;
    fly->angle = 0;
    fly->speed = 2;
    fly->destQue = XYQueueCreate(QUEUE_SIZE);
}

/*
 * ハエ構造体の利用終了
 */
void FlyDestroy(Fly *fly) {
    pthread_mutex_destroy(&fly->mutex);
    pthread_cond_destroy(&fly->cond);
    XYQueueDestroy(fly->destQue);
}

/*
 * ハエを移動する
 */
void FlyMove(Fly *fly) {
    int i;
    pthread_mutex_lock(&fly->mutex);
    fly->x += cos(fly->angle);
    fly->y += sin(fly->angle);
    pthread_mutex_unlock(&fly->mutex);
    requestDraw();
}

/*
 * ハエが指定座標にあるかどうか
 */
int FlyIsAt(Fly *fly, int x, int y) {
    int res;
    pthread_mutex_lock(&fly->mutex);
    res = ((int)(fly->x) == x) && ((int)(fly->y) == y);
    pthread_mutex_unlock(&fly->mutex);
    return res;
}

/*
 * 目標地点に合わせて移動方向と速度を調整する
 */
void FlySetDirection(Fly *fly, double destX, double destY) {
    pthread_mutex_lock(&fly->mutex);
    double dx = destX-fly->x;
    double dy = destY-fly->y;
    fly->angle = atan2(dy, dx);
    fly->speed = sqrt(dx*dx+dy*dy)/5.0;
    if(fly->speed < 2) /* あまり遅すぎると分かりづらいので */
        fly->speed = 2;
    pthread_mutex_unlock(&fly->mutex);
}

/*
 * 指定地点までの距離を得る
 */
double FlyDistance(Fly *fly, double x, double y) {
    double dx, dy, res;
    pthread_mutex_lock(&fly->mutex);
    dx = x-fly->x;
    dy = y-fly->y;
    res = sqrt(dx*dx+dy*dy);
    pthread_mutex_unlock(&fly->mutex);
    return res;
}

/*
 * ハエの目標地点をセットする
 */
int FlySetDestination(Fly *fly, double x, double y) {
    /* 直接目的地をセットするのではなく、キューに入れる */
    return XYQueueAdd(fly->destQue, x, y);
}

/*
 * 目標地点がセットされるまで最大msecミリ秒待ち、目標地点を *destX, *destYに入れる。
 * 目標地点がセットされたら1を、タイムアウトしたら0を返す
 */
int FlyWaitForSetDestination(Fly *fly, double *destX, double *destY, long msec) {
    if(!XYQueueWait(fly->destQue, msec))
        return 0;
    if(!XYQueueGet(fly->destQue, destX, destY))
        return 0;
    return 1;
}

/*
 * ハエを動かすスレッド
 */
static void *doMove(void *arg) {
    Fly *fly = (Fly *)arg;
    double destX, destY;
    while(!stopRequest) {
        /* 行き先がセットされるのを待つ */
        if(!FlyWaitForSetDestination(fly, &destX, &destY, 100))
            continue;
        /* 目標地点の方向をセット */
        FlySetDirection(fly, destX, destY);
        /* 行き先に到着するまで移動する */
        while((FlyDistance(fly, destX, destY) >= 1) && !stopRequest) {
            FlyMove(fly);
            mSleep((int)(1000.0/fly->speed));
        }
    }
    return NULL;
}

/*
 * スクリーンの描画を要請する
 */
static void requestDraw() {
    pthread_mutex_lock(&drawMutex);
    drawRequest = 1;
    pthread_cond_signal(&drawCond);
    pthread_mutex_unlock(&drawMutex);
}

/*
 * スクリーンを描画する
 */
static void drawScreen() {
    int x,y;
    char ch;
    int i;

    saveCursor();
    moveCursor(0, 0);
    for(y = 0; y < HEIGHT; y++) {
        for(x = 0; x < WIDTH; x++) {
            ch = 0;
            /* x,yの位置にあるハエがあればそのmarkを表示する */
            for(i = 0; i < MAX_FLY; i++) {
                if(FlyIsAt(&flyList[i], x, y)) {
                    ch = flyList[i].mark;
                    break;
                }
            }
            if(ch != 0) {
                putchar(ch);
            } else if((y == 0) || (y == HEIGHT-1)) {
                /* 上下の枠線を表示する */
                putchar('-');
            } else if((x == 0) || (x == WIDTH-1)) {
                /* 左右の枠線を表示する */
                putchar('|');
            } else {
                /* 枠線でもハエでもない */
                putchar(' ');
            }
        }
        putchar('\n');
    }
    restoreCursor();
    fflush(stdout);
}

/*
 * スクリーンを描画し続けるスレッド
 */
void *doDraw(void *arg) {
    int err;
    pthread_mutex_lock(&drawMutex);
    while(!stopRequest) {
        err = pthread_cond_timedwait_msec(&drawCond, &drawMutex, 100);
        if((err != 0) && (err != ETIMEDOUT)) {
            printf("Fatal error on pthread_cond_timedwait\n");
            exit(1);
        }
        while(drawRequest && !stopRequest) {
            drawRequest = 0;
            pthread_mutex_unlock(&drawMutex);
            drawScreen();
            pthread_mutex_lock(&drawMutex);
        }
    }
    pthread_mutex_unlock(&drawMutex);
    return NULL;
}

int main() {
    pthread_t drawThread;
    pthread_t moveThread;
    int i;
    char buf[40],*cp;
    double destX, destY;

    /* 初期化 */
    pthread_mutex_init(&drawMutex, NULL);
    pthread_cond_init(&drawCond, NULL);
    clearScreen();
    FlyInitCenter(&flyList[0], '@');

    /* ハエの動作処理 */
    pthread_create(&moveThread, NULL, doMove, (void *)&flyList[0]);

    /* 描画処理 */
    pthread_create(&drawThread, NULL, doDraw, NULL);
    requestDraw();

    /* メインスレッドは何か入力されるのを待ち、ハエの目標点をセットする */
    while(1) {
        printf("Destination? ");
        fflush(stdout);
        fgets(buf, sizeof(buf), stdin);
        if(strncmp(buf, "stop", 4) == 0) /* "stop"と入力するとプログラム終了 */
            break;
        /* 座標を読み取ってセットする */
        destX = strtod(buf, &cp);
        destY = strtod(cp, &cp);
        if(!FlySetDestination(&flyList[0], destX, destY)) {
            printf("The fly is busy now. Try later.\n");
        }
    }
    stopRequest = 1;
    
    /* スレッド撤収 */
    pthread_join(drawThread, NULL);
    pthread_join(moveThread, NULL);
    FlyDestroy(&flyList[0]);
    pthread_mutex_destroy(&drawMutex);
    pthread_cond_destroy(&drawCond);

    return 0;
}
