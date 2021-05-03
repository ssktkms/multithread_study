#include "intqueue.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>

/**
 * キュー管理構造体
 */
struct IntQueue_ {
    int *data;   /* データ配列 */
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
IntQueue *IntQueueCreate(size_t sz) {
    if(sz == 0)
        return NULL;
    /* キュー構造体作成 */
    IntQueue *que = (IntQueue *)malloc(sizeof(IntQueue));
    if(que == NULL)
        return NULL;
    /* データ配列作成 */
    que->size = sz+1;
    que->data = (int *)malloc(que->size*sizeof(int));
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
void IntQueueDestroy(IntQueue *que) {
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
size_t IntQueueGetSize(IntQueue *que) {
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
size_t IntQueueGetCount(IntQueue *que) {
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
size_t IntQueueGetFreeCount(IntQueue *que) {
    if(que == NULL)
        return 0;
    return que->size-1-IntQueueGetCount(que);
    /* sizeはcreateの時に決まったら変わることが無いのでロックの必要なし */
}

/**
 * 要素を追加する
 * @param que 対象キューへのポインタ
 * @param val 追加する値
 * @return 追加に成功した場合1, 失敗した場合0
 */
int IntQueueAdd(IntQueue *que, int val) {
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
    que->data[que->wp] = val;
    /* 追加位置更新 */
    que->wp = next_wp;
    /* IntQueueWaitに通知 */
    pthread_cond_signal(&que->cond);
    pthread_mutex_unlock(&que->mutex);
    return 1;
}

/**
 * 要素を取り出す
 * @param que 対象キューへのポインタ
 * @param val 取り出した値を格納する場所
 * @return 要素を取り出した場合1, 失敗した場合0
 */
int IntQueueGet(IntQueue *que, int *val) {
    if(que == NULL)
        return 0;
    /* rpを変更するのでロックする */
    pthread_mutex_lock(&que->mutex);
    if(que->rp == que->wp) { /* まだ読み出していないデータは無い */
        pthread_mutex_unlock(&que->mutex);
        return 0;
    }
    /* データ取り出し */
    if(val != NULL)
        *val = que->data[que->rp];
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
int IntQueueWait(IntQueue *que, long msec) {
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
