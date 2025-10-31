#ifndef FSTASK_H
#define FSTASK_H

#include <stddef.h>
#include "lfs.h"

/**
 * 電源オン時に呼ぶマウントと初期化関数
 * @return 0: 成功, 負の値: エラー
 */
int fstask_mount_and_init(void);

/**
 * 指定されたファイルを読み取る関数
 * @param filename 読み取るファイル名
 * @param buffer 読み取ったデータを格納するバッファ
 * @param buffer_size バッファのサイズ
 * @return 読み取ったバイト数、エラーの場合は負の値
 */
int fstask_read_file(const char *filename, char *buffer, size_t buffer_size);

/**
 * ディレクトリの内容をリストする関数
 * @param callback ファイル/ディレクトリ情報を処理するコールバック関数
 * @param user_data コールバック関数に渡すユーザーデータ
 * @return 0: 成功, 負の値: エラー
 */
int fstask_list_directory(void (*callback)(const char* name, int type, lfs_size_t size, void* user_data), void* user_data);

/**
 * ファイルを削除する関数
 * @param filename 削除するファイル名
 * @return 0: 成功, 負の値: エラー
 */
int fstask_remove_file(const char *filename);

/**
 * 指定されたファイルにバイナリデータを書き込む関数
 * @param filename 書き込むファイル名
 * @param data 書き込むデータ
 * @param data_size 書き込むデータのサイズ
 * @return 書き込んだバイト数、エラーの場合は負の値
 */
int fstask_write_file(const char *filename, const void *data, size_t data_size);

/**
 * ファイルのサイズを取得する関数
 * @param filename サイズを取得するファイル名
 * @return ファイルサイズ（バイト）、エラーの場合は負の値
 */
lfs_ssize_t fstask_get_file_size(const char *filename);

/**
 * ファイルシステムをアンマウントする関数
 * @return 0: 成功, 負の値: エラー
 */
int fstask_unmount(void);

#endif // FSTASK_H