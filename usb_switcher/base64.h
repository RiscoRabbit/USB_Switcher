#ifndef BASE64_H
#define BASE64_H

#include <stddef.h>
#include <stdint.h>

/**
 * Base64デコード関数
 * @param input Base64エンコードされた入力文字列
 * @param input_len 入力文字列の長さ
 * @param output デコード結果を格納するバッファ
 * @param output_len 出力バッファのサイズ
 * @return デコードしたバイト数、エラーの場合は負の値
 */
int base64_decode(const char* input, size_t input_len, uint8_t* output, size_t output_len);

/**
 * Base64エンコード関数
 * @param input エンコードする入力データ
 * @param input_len 入力データの長さ
 * @param output エンコード結果を格納するバッファ
 * @param output_len 出力バッファのサイズ
 * @return エンコードした文字数、エラーの場合は負の値
 */
int base64_encode(const uint8_t* input, size_t input_len, char* output, size_t output_len);

#endif // BASE64_H