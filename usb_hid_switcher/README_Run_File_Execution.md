# Run Command File Execution Feature

このドキュメントでは、`run`コマンドの新しいファイル実行機能について説明します。

## 概要

`run <filename>`コマンドが拡張され、LittleFSからLuaスクリプトファイルを読み取って実行できるようになりました。

## 機能の変更

### 以前の動作:
```
run keypress(0x04)  # 引数をそのままLuaコマンドとして実行
```

### 新しい動作:
```
run script.lua      # LittleFSから'script.lua'ファイルを読み取ってLuaスクリプトとして実行
```

## 使用方法

### 1. Luaスクリプトファイルの作成と保存

#### CDC経由でファイル作成:
```bash
receive script.lua
# Base64エンコードされたファイル内容を送信
# 空行で終了
```

#### ファイル例 (script.lua):
```lua
-- キーボード操作のサンプルスクリプト
print("Starting keyboard demo...")

-- 'Hello'を入力
print("Typing 'Hello'")
keypress(0x0B)  -- H
sleep(100)
keyrelease(0x0B)
sleep(50)

keypress(0x08)  -- E
sleep(100)
keyrelease(0x08)
sleep(50)

keypress(0x0F)  -- L
sleep(100)
keyrelease(0x0F)
sleep(50)

keypress(0x0F)  -- L
sleep(100)
keyrelease(0x0F)
sleep(50)

keypress(0x12)  -- O
sleep(100)
keyrelease(0x12)

print("Demo completed!")
```

### 2. スクリプトファイルの実行

```bash
run script.lua
```

### 3. 実行結果の確認

実行時のメッセージ例:
```
Lua script file 'script.lua' added to queue (1/10)
[Lua File] Loading and executing: script.lua
[Lua File] File loaded successfully (245 bytes)
[Lua File] Executing 'script.lua' (245 bytes)
Starting keyboard demo...
Typing 'Hello'
Key pressed: 0x0B
Key released: 0x0B
...
Demo completed!
[Lua File] 'script.lua' executed successfully
```

## 実装の詳細

### 新しい関数

#### `execute_lua_file(const char* filename)`
- LittleFSからファイルを読み取り
- ファイル内容をLuaスクリプトとして実行
- エラーハンドリングと詳細なログ出力

#### プロセスフロー
1. **コマンド受信**: `run script.lua`
2. **FIFO登録**: `FILE:script.lua`として内部キューに追加
3. **ファイル読取**: LittleFSから最大8KB読み取り
4. **スクリプト実行**: Luaインタープリターで実行
5. **結果出力**: CDC/UARTに実行結果を表示

### エラーハンドリング

#### ファイルが見つからない場合:
```
[Lua File Error] Failed to read file 'missing.lua' (error: -2)
```

#### Lua構文エラーの場合:
```
[Lua File Error] script.lua:5: syntax error near 'invalid'
```

#### メモリ不足の場合:
```
[Lua File Error] Failed to allocate memory for file reading
```

## 利点

1. **ファイル管理**: 複雑なスクリプトをファイルとして保存・管理
2. **再利用性**: 同じスクリプトを繰り返し実行可能
3. **バージョン管理**: 異なるバージョンのスクリプトを保存可能
4. **デバッグ支援**: ファイル読み込み〜実行まで詳細なログ

## 使用例

### 基本的なキーボード操作:
```lua
-- basic_key.lua
keypress(0x04)  -- A
sleep(10)
keyrelease(0x04)
```

### ショートカットキー実行:
```lua
-- ctrl_a.lua
print("Executing Ctrl+A (Select All)")
keypress(0xE0)  -- Left Ctrl
keypress(0x04)  -- A
sleep(100)
keyrelease(0x04)
keyrelease(0xE0)
print("Ctrl+A executed")
```

### 文字列入力:
```lua
-- type_text.lua
function type_char(keycode)
    keypress(keycode)
    sleep(50)
    keyrelease(keycode)
    sleep(50)
end

-- "Hi"を入力
type_char(0x0B)  -- H
type_char(0x0C)  -- I
```

## ファイル制限

- **最大ファイルサイズ**: 8KB (8192バイト)
- **ファイル形式**: UTF-8テキスト推奨
- **ファイル名**: LittleFSの制限に従う

## コマンド一覧（更新）

```
version         - Show version information
run <filename>  - Execute Lua script file from LittleFS
queue           - Show queue status  
ls              - List filesystem contents
rm <filename>   - Remove specified file
cat <filename>  - Display file contents
receive <filename> - Start file receive mode
rcv <filename>     - Alias for receive command
```

この機能により、USB HID Switcher でより柔軟で再利用可能なキーボード操作スクリプトの管理と実行が可能になりました。