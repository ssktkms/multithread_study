#ifndef INTQUEUE_H
#define INTQUEUE_H

#include <stdlib.h>

/**
 * キュー型（仮宣言）
 */
typedef struct IntQueue_ IntQueue;


/**
 * キューを作る
 * @param sz 最大要素数
 * @return 作成したキューへのポインタ。キュー作成に失敗した場合 NULL
 */
extern IntQueue *IntQueueCreate(size_t sz);

/**
 * キューを削除する
 * @param que 削除するキューへのポインタ
 */
extern void IntQueueDestroy(IntQueue *que);

/**
 * キューの最大要素数を得る
 * @param que 対象キューへのポインタ
 * @return 最大要素数
 */
extern size_t IntQueueGetSize(IntQueue *que);

/**
 * 現在の登録要素数を得る
 * @param que 対象キューへのポインタ
 * @return 現在の登録要素数
 */
extern size_t IntQueueGetCount(IntQueue *que);

/**
 * 現在の空き要素数を得る
 * @param que 対象キューへのポインタ
 * @return 空き要素数
 */
extern size_t IntQueueGetFreeCount(IntQueue *que);

/**
 * 要素を追加する
 * @param que 対象キューへのポインタ
 * @param val 追加する値
 * @return 追加に成功した場合1, 失敗した場合0
 */
extern int IntQueueAdd(IntQueue *que, int val);

/**
 * 要素を取り出す
 * @param que 対象キューへのポインタ
 * @param val 取り出した値を格納する場所
 * @return 要素を取り出した場合1, 失敗した場合0
 */
extern int IntQueueGet(IntQueue *que, int *val);

/**
 * 要素が追加されるのを最大msecミリ秒待つ
 * すでに要素がある場合には待たずにすぐ1を返す。
 * @param que 対象キューへのポインタ
 * @param msec 最大待ち時間
 * @return 要素が追加された場合1, タイムアウトした場合0
 */
extern int IntQueueWait(IntQueue *que, long msec);

#endif /* INTQUEUE_H */
