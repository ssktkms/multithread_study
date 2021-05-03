#include "xyqueue.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>

/**
 * キューの要素の型
 */
typedef struct {
    double x;
    double y;
} XYQueueItem;

/**
 * キュー管理構造体
 */
struct XYQueue_ {
    XYQueueItem *data;   /* データ配列 */
    size_t size;       /* データ配列サイズ（最大要素数+1）*/
    size_t wp;         /* 追加位置 */
    size_t rp;         /* 読出位置 */
    /* キュー処理の排他制御を内包させる */
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};


/**
 * キューを作る
 * @param sz 最大要素数
 * @return 作成したキューへのポインタ。キュー作成に失敗した場合 NULL
 */
XYQueue *XYQueueCreate(size_t sz) {
    if(sz == 0)
        return NULL;
    /* キュー構造体作成 */
    XYQueue *que = (XYQueue *)malloc(sizeof(XYQueue));
    if(que == NULL)
        return NULL;
    /* データ配列作成 */
    que->size = sz+1;
    que->data = (XYQueueItem *)malloc(que->size*sizeof(XYQueueItem));
    if(que->data == NULL) {
        free(que);
        return NULL;
    }
    /* 追加位置、読出位置初期化 */
    que->wp = que->rp = 0;
    /* ミューテックス、条件変数初期化 */
    pthread_mutex_init(&que->mutex, NULL);
    pthread_cond_init(&que->cond, NULL);
    return que;
}

/**
 * キューを削除する
 * @param que 削除するキューへのポインタ
 */
void XYQueueDestroy(XYQueue *que) {
    if(que == NULL)
        return;
    free(que->data);
    pthread_mutex_destroy(&que->mutex);
    pthread_cond_destroy(&que->cond);
    free(que);
}

/**
 * キューの最大要素数を得る
 * @param que 対象キューへのポインタ
 * @return 最大要素数
 */
size_t XYQueueGetSize(XYQueue *que) {
    if(que == NULL)
        return 0;
    return que->size-1;
    /* sizeはcreateの時に決まったら変わることが無いのでロックの必要なし */
}

/**
 * 現在の登録要素数を得る
 * @param que 対象キューへのポインタ
 * @return 現在の登録要素数
 */
size_t XYQueueGetCount(XYQueue *que) {
    if(que == NULL)
        return 0;
    size_t count;
    pthread_mutex_lock(&que->mutex);
    /* wpとrpの読み取りの間に値が変わるとまずいのでロックする */
    if(que->wp < que->rp) /* 追加位置〜読出位置がバッファ境界をまたいでいる */
        count = que->size+que->wp-que->rp;
    else
        count = que->wp-que->rp;
    pthread_mutex_unlock(&que->mutex);
    return count;
}

/**
 * 現在の空き要素数を得る
 * @param que 対象キューへのポインタ
 * @return 空き要素数
 */
size_t XYQueueGetFreeCount(XYQueue *que) {
    if(que == NULL)
        return 0;
    return que->size-1-XYQueueGetCount(que);
    /* sizeはcreateの時に決まったら変わることが無いのでロックの必要なし */
}

/**
 * 要素を追加する
 * @param que 対象キューへのポインタ
 * @param x X座標値
 * @param y Y座標値
 * @return 追加に成功した場合1, 失敗した場合0
 */
int XYQueueAdd(XYQueue *que, double x, double y) {
    if(que == NULL)
        return 0;
    /* wpを変更するのでロックする */
    pthread_mutex_lock(&que->mutex);
    size_t next_wp = que->wp+1;
    if(next_wp >= que->size) /* バッファ境界を越えた */
        next_wp -= que->size;
    if(next_wp == que->rp) {
        /* 追加すると読出位置に追いついてしまう場合、最大要素数オーバー */
        pthread_mutex_unlock(&que->mutex);
        return 0;
    }
    /* 追加位置にデータを書き込む */
    que->data[que->wp].x = x;
    que->data[que->wp].y = y;
    /* 追加位置更新 */
    que->wp = next_wp;
    /* XYQueueWaitに通知 */
    pthread_cond_signal(&que->cond);
    pthread_mutex_unlock(&que->mutex);
    return 1;
}

/**
 * 要素を取り出す
 * @param que 対象キューへのポインタ
 * @param x 取り出したX座標値を格納する場所
 * @param y 取り出したY座標値を格納する場所
 * @return 要素を取り出した場合1, 失敗した場合0
 */
int XYQueueGet(XYQueue *que, double *x, double *y) {
    if(que == NULL)
        return 0;
    /* rpを変更するのでロックする */
    pthread_mutex_lock(&que->mutex);
    if(que->rp == que->wp) { /* まだ読み出していないデータは無い */
        pthread_mutex_unlock(&que->mutex);
        return 0;
    }
    /* データ取り出し */
    if(x != NULL)
        *x = que->data[que->rp].x;
    if(y != NULL)
        *y = que->data[que->rp].y;
    /* 読出位置更新 */
    if(++(que->rp) >= que->size) /* バッファ境界を越えた */
        que->rp -= que->size;
    pthread_mutex_unlock(&que->mutex);
    return 1;
}

/**
 * 要素が追加されるのを最大msecミリ秒待つ
 * すでに要素がある場合には待たずにすぐ1を返す。
 * @param que 対象キューへのポインタ
 * @param msec 最大待ち時間
 * @return 要素が追加された場合1, タイムアウトした場合0
 */
int XYQueueWait(XYQueue *que, long msec) {
    if(que == NULL)
        return 0;
    /* タイムアウトする時刻を計算する */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += msec/1000;
    ts.tv_nsec += (msec%1000)*1000000;
    if(ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }
    /* 条件待ちの前にロック */
    pthread_mutex_lock(&que->mutex);
    while(que->wp == que->rp) {
        /* wpとrpが同じということは読み出していないデータが無いということ。
           signalを待つ */
        int err = pthread_cond_timedwait(&que->cond, &que->mutex, &ts);
        if(err == ETIMEDOUT) {
            /* タイムアウト */
            break;
        } else if(err != 0) {
            /* 0でもETIMEDOUTでもないのは致命的なエラー */
            fprintf(stderr, "Fatal error on pthread_cond_timedwait.\n");
            exit(1);
        }
        /* 条件変数がONになったが読出しデータが無いときは、再び待つ。*/
    }
    /* アンロックする前に読出しデータがあるかどうかを確認しておく */
    int res = (que->wp != que->rp);
    pthread_mutex_unlock(&que->mutex);
    return res;
}
