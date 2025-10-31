# USB HID Keyboard Lua Functions

このドキュメントでは、LuaTask.cに追加された新しいUSB HIDキーボード関数について説明します。

## 追加された関数

### 1. keypress(keycode)
**説明**: 指定されたHIDキーコードのキーを押下します。

**引数**:
- `keycode` (number): HIDキーコード (0-255の範囲)

**例**:
```lua
keypress(0x04)  -- 'A'キーを押下
keypress(0x05)  -- 'B'キーを押下
```

**注意**:
- 同じキーを複数回押下しても、既に押下されている場合は無視されます
- 最大6つのキーを同時に押下できます（HID仕様の制限）
- 6つを超えるキーを押下しようとするとエラーになります

### 2. keyrelease(keycode)
**説明**: 指定されたHIDキーコードのキーを離します。

**引数**:
- `keycode` (number): HIDキーコード (0-255の範囲)

**例**:
```lua
keyrelease(0x04)  -- 'A'キーを離す
keyrelease(0x05)  -- 'B'キーを離す
```

**注意**:
- 押下されていないキーを離そうとしても無視されます

## HIDキーコード参照

### 主要な文字キー:
```
A = 0x04    B = 0x05    C = 0x06    D = 0x07
E = 0x08    F = 0x09    G = 0x0A    H = 0x0B
I = 0x0C    J = 0x0D    K = 0x0E    L = 0x0F
M = 0x10    N = 0x11    O = 0x12    P = 0x13
Q = 0x14    R = 0x15    S = 0x16    T = 0x17
U = 0x18    V = 0x19    W = 0x1A    X = 0x1B
Y = 0x1C    Z = 0x1D
```

### 数字キー:
```
1 = 0x1E    2 = 0x1F    3 = 0x20    4 = 0x21    5 = 0x22
6 = 0x23    7 = 0x24    8 = 0x25    9 = 0x26    0 = 0x27
```

### 特殊キー:
```
ENTER    = 0x28    ESCAPE   = 0x29    BACKSPACE = 0x2A
TAB      = 0x2B    SPACE    = 0x2C    CAPS_LOCK = 0x39
F1       = 0x3A    F2       = 0x3B    F3        = 0x3C
F4       = 0x3D    F5       = 0x3E    F6        = 0x3F
```

## 使用例

### 基本的な使い方:
```lua
-- 'H' 'E' 'L' 'L' 'O' を順番に入力
keypress(0x0B)  -- H
sleep(100)
keyrelease(0x0B)
keypress(0x08)  -- E
sleep(100)
keyrelease(0x08)
keypress(0x0F)  -- L
sleep(100)
keyrelease(0x0F)
keypress(0x0F)  -- L
sleep(100)
keyrelease(0x0F)
keypress(0x12)  -- O
sleep(100)
keyrelease(0x12)
```

### 同時押し:
```lua
-- Ctrl+A（全選択）を実行
keypress(0xE0)  -- Left Ctrl
keypress(0x04)  -- A
sleep(100)      -- 少し待機
keyrelease(0x04)  -- A を離す
keyrelease(0xE0)  -- Ctrl を離す
```

### 長押し:
```lua
-- Aキーを2秒間押し続ける
keypress(0x04)
sleep(2000)
keyrelease(0x04)
```

## デバッグ機能

両関数は実行時にCDCインターフェース経由でデバッグメッセージを出力します:
```
Key pressed: 0x04
Key released: 0x04
```

## 実装の詳細

- キーボード状態は内部的にグローバル変数で管理されます
- 最大6つのキーコードを同時に保持できます
- USB HIDレポートはキー操作のたびに自動的に送信されます
- HIDインターフェース0（キーボード）、レポートID 1を使用します

## 注意事項

1. **キーコード範囲**: 0-255の範囲外の値はエラーになります
2. **同時押しの制限**: HID仕様により最大6つのキーまで同時押し可能
3. **デバイスの準備**: HIDインターフェースが準備できていない場合、レポート送信は無視されます
4. **メモリ管理**: 不適切な引数でLuaエラーが発生する可能性があります