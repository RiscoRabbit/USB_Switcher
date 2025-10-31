# FIFO Queue Lua Command Execution

このドキュメントでは、FIFOキューからのコマンドをLuaスクリプトとして実行する新機能について説明します。

## 概要

`process_fifo_commands`関数が拡張され、FIFOキューから受信したコマンドをLuaスクリプトとして実行できるようになりました。

## 新機能

### 1. **init_fifo_lua_state()**
- FIFO用の専用Lua状態を初期化
- Luaライブラリとカスタム関数（keypress、keyrelease、sleep）を登録
- 一度だけ呼び出される（`task2_function`内で自動実行）

### 2. **execute_lua_command(const char* lua_command)**
- 受信したコマンド文字列をLuaスクリプトとして実行
- エラーハンドリングとデバッグ出力を含む
- 戻り値の処理（数値、文字列対応）

### 3. **process_fifo_commands()** (拡張版)
- FIFOキューから取得したコマンドをLuaスクリプトとして実行
- UARTへのデバッグ出力も継続
- エラー処理とレスポンシブな実行

## 使用例

### FIFOキューに送信できるLuaコマンド例:

```lua
-- 単一のキー操作
keypress(0x04); keyrelease(0x04)

-- 複数のキー操作
keypress(0x04); sleep(100); keypress(0x05); keyrelease(0x04); keyrelease(0x05)

-- 組み合わせキー (Ctrl+A)
keypress(0xE0); keypress(0x04); sleep(50); keyrelease(0x04); keyrelease(0xE0)

-- 単語の入力 (Hello)
keypress(0x0B); sleep(50); keyrelease(0x0B); keypress(0x08); sleep(50); keyrelease(0x08)

-- 計算とprint文
x = 2 + 3; print('Result:', x)
```

### CDCインターフェース経由での送信例:
```
keypress(0x04)
sleep(500)  
keyrelease(0x04)
```

## 実行フロー

1. **初期化** (`task2_function`開始時)
   ```c
   init_fifo_lua_state();  // FIFO用Lua状態を初期化
   ```

2. **コマンド受信** (USBタスクやCDCからFIFOへ)
   ```c
   fifo_push("keypress(0x04)");
   ```

3. **コマンド実行** (100ms間隔でポーリング)
   ```c
   process_fifo_commands();
   -> execute_lua_command("keypress(0x04)");
   ```

## デバッグ出力

### 正常実行時:
```
[Lua] Executing: keypress(0x04)
Key pressed: 0x04
[Lua] Command executed successfully
```

### エラー時:
```
[Lua] Executing: keypress(-1)
[Lua Error] Invalid keycode: -1 (must be 0-255)
```

### 戻り値がある場合:
```
[Lua] Executing: return 42
[Lua] Returned: 42.000000
[Lua] Command executed successfully
```

## メリット

1. **リアルタイム実行**: FIFOキューを通じて即座にキーボード操作を実行
2. **エラー処理**: 不正なコマンドやLua構文エラーを適切に処理
3. **デバッグ支援**: UART/CDC両方への詳細な出力
4. **拡張性**: 新しいLua関数を簡単に追加可能
5. **状態管理**: 専用のLua状態でキーボード状態を維持

## 注意事項

1. **メモリ使用量**: 追加のLua状態がRAMを使用
2. **コマンド長制限**: 64文字までのコマンド（MAX_COMMAND_LENGTH）
3. **実行間隔**: 100ms間隔でFIFOをポーリング（50ms実行遅延付き）
4. **エラー継続**: Luaエラーが発生しても次のコマンドは実行継続

## 実装ファイル

- **LuaTask.c**: メイン実装
- **LuaTask.h**: 関数宣言
- **USBtask.c**: FIFOキューへのコマンド送信（既存）
- **CDCCmd.c**: CDC経由でのコマンド受信（既存）

この機能により、USB HIDキーボードの動的制御がLuaスクリプトを通じて可能になり、リアルタイムでの柔軟なキーボード操作が実現できます。