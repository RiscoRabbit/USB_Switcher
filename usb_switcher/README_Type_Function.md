# Lua Type Function Documentation

このドキュメントでは、新しく追加された`type`関数について説明します。

## 概要

`type(text, delay_ms)`関数は、文字列を受け取り、各文字を適切なUSB HIDキーコードに変換して自動的にタイピングシミュレーションを行います。

## 関数仕様

```lua
type(text, delay_ms)
```

### 引数

- `text` (string): タイプする文字列
- `delay_ms` (number): キー押下時間と次の文字までの待機時間（ミリ秒）

### 戻り値

なし

## 機能詳細

### 自動文字マッピング

- **小文字 (a-z)**: 直接HIDキーコードにマッピング
- **大文字 (A-Z)**: Shiftキーと組み合わせて自動処理
- **数字 (0-9)**: 直接マッピング
- **特殊文字**: Shiftが必要な文字は自動的にShift+キーで処理

### サポートされる文字

#### 基本文字
```
a-z, A-Z, 0-9
```

#### 特殊文字（Shiftなし）
```
- = [ ] \ ; ' ` , . / スペース タブ エンター
```

#### 特殊文字（Shift必要）
```
! @ # $ % ^ & * ( ) _ + { } | : " ~ < > ?
```

## 使用例

### 基本的な使用方法
```lua
-- 10msの間隔で"Hello"をタイプ
type("Hello", 10)
```

### 大文字を含む例
```lua
-- 50msの間隔で"Hello World!"をタイプ（HとWは自動的にShift処理）
type("Hello World!", 50)
```

### 特殊文字を含む例
```lua
-- 100msの間隔でユーザー名とパスワードをタイプ
type("User@123", 100)     -- @は自動的にShift+2で処理
type("Pass#456!", 75)     -- #は自動的にShift+3、!はShift+1で処理
```

### プログラミングコードの例
```lua
-- C言語のコードをタイプ
type('#include <stdio.h>', 50)
type('int main() {', 50)
type('    printf("Hello\\n");', 50)
type('    return 0;', 50)
type('}', 50)
```

## 実行の流れ

### 各文字の処理手順

1. **文字解析**: 文字をHIDキーコードとShift必要性に変換
2. **Shift処理**: 必要に応じてShiftキーを押下
3. **キー押下**: 対象文字のキーを押下
4. **待機**: 指定されたdelay_ms時間待機
5. **キー解放**: キーを解放（Shiftも含む）
6. **次文字待機**: 次の文字まで指定時間待機

### タイミング図
```
文字 'H' の場合（Shift必要）:
0ms     : Shiftキー押下
0ms     : Hキー押下  
10ms    : Hキー解放
10ms    : Shiftキー解放
20ms    : 次の文字（'e'）処理開始
```

## エラーハンドリング

### サポートされていない文字
```
Warning: Unknown character '€' (0xE2) - skipping
```

### 遅延時間の制限
```lua
type("test", -1)     -- エラー: Invalid delay: -1 (must be 0-10000 ms)
type("test", 15000)  -- エラー: Invalid delay: 15000 (must be 0-10000 ms)
```

### 同時押しキー制限
```
Warning: Cannot type character - maximum simultaneous keys reached
```

## デバッグ出力

### 実行時のメッセージ
```
Typing text: 'Hello' with 10ms delay
Typing: 'Hello' (delay: 10ms)
Text typing completed
```

### CDCインターフェース出力
```
Typing: 'Hello' (delay: 10ms)
Text typing completed
```

## 実装の特徴

### メモリ効率
- 静的な文字マッピングテーブルを使用
- 動的メモリ割り当てなし

### パフォーマンス
- 高速な文字検索（線形検索、約100文字）
- FreeRTOSタスク遅延を使用した正確なタイミング

### 信頼性
- HIDインターフェース準備状態の確認
- キーボード状態の適切な管理
- エラー処理とログ出力

## 制限事項

1. **文字セット**: ASCII文字のサブセットのみサポート
2. **同時キー数**: 最大6キーまで（HID仕様）
3. **遅延範囲**: 0-10000ms
4. **国際文字**: 日本語、アクセント文字などは未サポート

## 拡張可能性

### 新しい文字の追加
```c
// char_to_keycode配列に新しいエントリを追加
{'€', 0x???, true},  // 新しい特殊文字
```

### カスタムレイアウト対応
- 異なるキーボードレイアウト（QWERTY以外）のサポート
- 地域固有の文字セットの追加

この`type`関数により、複雑な文字列も簡単に自動タイピングでき、USB HID Switcherでのテキスト入力自動化が大幅に向上します。