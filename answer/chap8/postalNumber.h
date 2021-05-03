#ifndef POSTALNUMBER_H
#define POSTALNUMBER_H

#include <stddef.h>

/**
 * 郵便番号データベースレコード構造体
 */
typedef struct {
    char code[16];   /* 郵便番号 */
    char pref[128];   /* 都道府県名 */
    char city[256];  /* 市区町村名 */
    char town[256];  /* 町域名 */
} PostalNumber;

/**
 * 郵便番号データベースを取り込む
 * returns: 取り込んだレコード数
 */
extern size_t PostalNumberLoadDB(void);

/**
 * 郵便番号がkeyに一致するか、または都道府県名、市区町村名、町域名のいずれかに
 * keyを含むレコードを探す
 * key: 検索する文字列
 * result: 結果を格納する配列
 * resultSize: resultの要素数
 * returns: 該当レコードの数
 */
extern size_t PostalNumberSearch(const char *key, PostalNumber *result, size_t resultSize);


#endif /* POSTALNUMBER_H */
