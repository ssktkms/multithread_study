#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define WIDTH 78   /* スクリーン幅 */
#define HEIGHT 23  /* スクリーン高さ */
#define MAX_FLY 6  /* 描画するハエの数 */
const char *flyMarkList = "o@*+.#"; /* ハエの描画文字一覧 */
#define DRAW_CYCLE 50   /* 描画周期（ミリ秒）*/
#define MIN_SPEED 1.0   /* ハエの最低移動速度 */
#define MAX_SPEED 20.0  /* ハエの最大移動速度 */

int stopRequest;  /* スレッド終了フラグ */

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
 * minValue以上maxValue未満のランダム値を得る
 */
double randDouble(double minValue, double maxValue) {
    return minValue+(double)rand()/((double)RAND_MAX+1)*(maxValue-minValue);
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
 * ハエ構造体
 */
typedef struct {
    char mark;    /* 表示キャラクタ */
    double x, y;  /* 座標 */
    double angle; /* 移動方向（角度）*/
    double speed; /* 移動速度（ピクセル/秒）*/
    pthread_mutex_t mutex;
} Fly;

Fly flyList[MAX_FLY];

/*
 * ハエの状態をランダムに初期化
 */
void FlyInitRandom(Fly *fly, char mark_) {
    fly->mark = mark_;
    pthread_mutex_init(&fly->mutex, NULL);
    fly->x = randDouble(0, (double)(WIDTH-1));
    fly->y = randDouble(0, (double)(HEIGHT-1));
    fly->angle = randDouble(0, M_2_PI);
    fly->speed = randDouble(MIN_SPEED, MAX_SPEED);
}

/*
 * ハエ構造体の利用終了
 */
void FlyDestroy(Fly *fly) {
    pthread_mutex_destroy(&fly->mutex);
}

/*
 * 指定座標からの距離を求める
 */
double FlyDistance(Fly *fly, double x, double y) {
    double dx,dy,d;
    pthread_mutex_lock(&fly->mutex);
    dx = x-fly->x;
    dy = y-fly->y;
    d = sqrt(dx*dx+dy*dy);
    pthread_mutex_unlock(&fly->mutex);
    return d;
}

/*
 * ハエを移動する
 */
void FlyMove(Fly *fly) {
    int i;
    pthread_mutex_lock(&fly->mutex);
    fly->x += cos(fly->angle);
    fly->y += sin(fly->angle);
    /* X方向の縁にぶつかったら方向を変える */
    if(fly->x < 0) {
        fly->x = 0;
        fly->angle = M_PI-fly->angle; 
    } else if(fly->x > WIDTH-1) {
        fly->x = WIDTH-1;
        fly->angle = M_PI-fly->angle; 
    }
    /* Y方向の縁にぶつかったら方向を変える */
    if(fly->y < 0) {
        fly->y = 0;
        fly->angle = -fly->angle;
    } else if(fly->y > HEIGHT-1) {
        fly->y = HEIGHT-1;
        fly->angle = -fly->angle;
    }
    /* 他のハエのすぐ近くに来たら180度方向を変える */
    for(i = 0; i < MAX_FLY; i++) {
        if((flyList[i].mark != fly->mark) && (FlyDistance(&flyList[i], fly->x, fly->y) < 2.0)) {
            fly->angle += M_PI;
            break;
        }
    }
    pthread_mutex_unlock(&fly->mutex);
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
 * ハエを動かし続けるスレッド
 */
void *doMove(void *arg) {
    Fly *fly = (Fly *)arg;
    while(!stopRequest) {
        FlyMove(fly);
        mSleep((int)(1000.0/fly->speed));
    }
    return NULL;
}

/*
 * スクリーンを描画する
 */
void drawScreen() {
    int x,y;
    char ch;
    int i;

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
}

/*
 * スクリーンを描画し続けるスレッド
 */
void *doDraw(void *arg) {
    while(!stopRequest) {
        drawScreen();
        mSleep(DRAW_CYCLE);
    }
    return NULL;
}

int main() {
    pthread_t drawThread;
    pthread_t moveThread[MAX_FLY];
    int i;
    char buf[40];

    /* 初期化 */
    srand((unsigned int)time(NULL));
    clearScreen();
    for(i = 0; i < MAX_FLY; i++) {
        FlyInitRandom(&flyList[i], flyMarkList[i]);
    }

    /* ハエの動作処理 */
    for(i = 0; i < MAX_FLY; i++) {
        pthread_create(&moveThread[i], NULL, doMove, (void *)&flyList[i]);
    }
    /* 描画処理 */
    pthread_create(&drawThread, NULL, doDraw, NULL);

    /* メインスレッドは何か入力されるのを待つだけ */
    fgets(buf, sizeof(buf), stdin);
    stopRequest = 1;
    
    /* スレッド撤収 */
    pthread_join(drawThread, NULL);
    for(i = 0; i < MAX_FLY; i++) {
        pthread_join(moveThread[i], NULL);
        FlyDestroy(&flyList[i]);
    }

    return 0;
}
