#ifndef XYQUEUE_H
#define XYQUEUE_H

#include <stdlib.h>

/**
 * キュー型（仮宣言）
 */
typedef struct XYQueue_ XYQueue;


/**
 * キューを作る
 * @param sz 最大要素数
 * @return 作成したキューへのポインタ。キュー作成に失敗した場合 NULL
 */
extern XYQueue *XYQueueCreate(size_t sz);

/**
 * キューを削除する
 * @param que 削除するキューへのポインタ
 */
extern void XYQueueDestroy(XYQueue *que);

/**
 * キューの最大要素数を得る
 * @param que 対象キューへのポインタ
 * @return 最大要素数
 */
extern size_t XYQueueGetSize(XYQueue *que);

/**
 * 現在の登録要素数を得る
 * @param que 対象キューへのポインタ
 * @return 現在の登録要素数
 */
extern size_t XYQueueGetCount(XYQueue *que);

/**
 * 現在の空き要素数を得る
 * @param que 対象キューへのポインタ
 * @return 空き要素数
 */
extern size_t XYQueueGetFreeCount(XYQueue *que);

/**
 * 要素を追加する
 * @param que 対象キューへのポインタ
 * @param x X座標値
 * @param y Y座標値
 * @return 追加に成功した場合1, 失敗した場合0
 */
extern int XYQueueAdd(XYQueue *que, double x, double y);

/**
 * 要素を取り出す
 * @param que 対象キューへのポインタ
 * @param x 取り出したX座標値を格納する場所
 * @param y 取り出したY座標値を格納する場所
 * @return 要素を取り出した場合1, 失敗した場合0
 */
extern int XYQueueGet(XYQueue *que, double *x, double *y);

/**
 * 要素が追加されるのを最大msecミリ秒待つ
 * すでに要素がある場合には待たずにすぐ1を返す。
 * @param que 対象キューへのポインタ
 * @param msec 最大待ち時間
 * @return 要素が追加された場合1, タイムアウトした場合0
 */
extern int XYQueueWait(XYQueue *que, long msec);

#endif /* XYQUEUE_H */
