# USB HID Switcher

高度なUSB HID切り替え装置です。二個のRaspberry Pi Picoを使用して、複数のPCを切り替えながらUSB Keyboardやマウス操作ができます。Luaスクリプトにより、マクロ実行も可能です。
Advanced USB HID switching device. Uses two Raspberry Pi Picos to enable USB keyboard and mouse operation while switching between multiple PCs. Macro execution is also possible through Lua scripts.

## 使い方

### 初期セットアップ

#### 1. ファームウェアの書き込み

1. **BOOTSELモード**でPicoをPCに接続します
   - BOOTSELボタンを押しながらUSBケーブルを接続
   - PicoがRPI-RP2ドライブとして認識されます

2. **ファームウェアファイルのコピー**
   ```bash
   # ビルドしたuf2ファイルをPicoにコピー
   cp build/usb_hid_switcher/usb_hid_switcher.uf2 /media/RPI-RP2/
   ```
   
3. **自動再起動**
   - ファイルコピー完了後、Picoが自動的に再起動します
   - USB HID SwitcherとしてPCに認識されます

#### 2. 初期設定（COMポート経由）

ファームウェア書き込み後、PicoはCDC（仮想COMポート）として認識されます。
任意のターミナルソフト（PuTTY、TeraTermなど）で接続してください。

**COMポート設定:**
- ボーレート: 115200
- データビット: 8
- パリティ: なし
- ストップビット: 1

#### 3. configファイルの作成

**一台目のPico（DEVICEID=1）:**
```bash
# COMポートに接続後、以下を実行
prog config

# 以下の内容を入力
PROTOCOL=REPORT
DEVICEID=0

# Ctrl+D で保存・終了
```

**二台目のPico（DEVICEID=2）:**
```bash
# COMポートに接続後、以下を実行
prog config

# 以下の内容を入力
PROTOCOL=REPORT
DEVICEID=1

# Ctrl+D で保存・終了
```

#### 4. 設定確認
```bash
# 設定ファイルの確認
cat config

# システム情報確認
version
```

#### 5. 切り替えマクロの設定

PC間の切り替えを行うため、以下のLuaマクロをそれぞれ作成してください。

**切り替えマクロ1（Meta-M）:**
```bash
# COMポートに接続後、以下を実行
prog Meta-M

# 以下の内容を入力
switch(0)
oled_clear()
oled_text(8, 12, 2, "SWITCH 0")
oled_update()

# Ctrl+D で保存・終了
```

**切り替えマクロ2（Meta-N）:**
```bash
# COMポートに接続後、以下を実行
prog Meta-N

# 以下の内容を入力
switch(1)
oled_clear()
oled_text(8, 12, 2, "SWITCH 1")
oled_update()

# Ctrl+D で保存・終了
```

#### 6. 切り替え操作

マクロ設定完了後、以下のキー操作でPC間の切り替えが可能です：

- **CapsLock + M**: PC1（SWITCH 0）に切り替え
- **CapsLock + N**: PC2（SWITCH 1）に切り替え

**確認方法:**
```bash
# 作成したマクロファイルの確認
cat Meta-M
cat Meta-N
```


## 主な機能

### 🔄 **デュアルUSBモード**
- **USB Host**: 複数のUSB HIDデバイスを同時接続、USB Hubの使用も可能
- **USB Device**: キーボード、マウス、ゲームパッドとしてPCに認識

### 🎮 **対応デバイス**
- **キーボード**: BOOTプロトコル、REPORTプロトコル対応
- **マウス**: 16bit高精度、ホイール・パン操作対応
- **ゲームパッド**: 16ボタン、2軸スティック、ハットスイッチ対応

### 🧠 **Luaスクリプト統合**
- 自動タイピング機能（`type`関数）
- FIFOキューを使った非同期コマンド実行
- ファイルシステムからのスクリプト自動実行

### 🎨 **視覚的フィードバック**
- **WS2812 RGB LED**: デバイス状態の色分け表示
- **OLED ディスプレイ**: 接続デバイス情報、設定状況表示
- **動的な状態変化**: アクティブ/アイドル/スリープ状態

### 📡 **通信インターフェース**
- **CDC (Virtual COM Port)**: USB経由でPCから仮想COMポートとして認識され、設定やコマンド送信が可能
- **UART0**: デバッグ出力
- **UART1**: Pico間通信
- **File System**: LittleFSによる設定・スクリプト保存

### 🔧 **高度な機能**
- デバイス別プロファイル設定
- メタキー機能（キー組み合わせの動的変更）

## ハードウェア構成

### 対応ボード
- **Raspberry Pi Pico** (RP2040)
- **Raspberry Pi Pico 2** (RP2350)は未対応

### ピン配置

#### Pico (RP2040)
```
GPIO 9-10  : USB Host 1 (D+/D-)
GPIO 11-12 : USB Host 2 (D+/D-)
GPIO 16    : WS2812 RGB LED
GPIO 4-5   : UART1
GPIO 0-1   : UART0 (デバッグ/通信)
GPIO 26    : I2C1 SDA (OLED Display)
GPIO 27    : I2C1 SCL (OLED Display)
```

## ビルド方法

### 1. 依存関係
```bash
# Pico SDK (v1.5.0以降推奨)
export PICO_SDK_PATH=/path/to/pico-sdk

# 必要なツール
sudo apt install cmake gcc-arm-none-eabi build-essential
```

### 2. ビルド手順
```bash
# プロジェクトディレクトリに移動
cd USB_HID_Switcher
mkdir build && cd build
cmake .. -DPICO_BOARD=pico -DPICO_SDK_PATH=$PICO_SDK_PATH
# ビルド実行
make
```

### 3. ファームウェア書き込み
```bash
# BOOTSELモードでPicoを接続後
cp usb_hid_switcher/usb_hid_switcher.uf2 /media/RPI-RP2/
```

## 使用方法

### 基本設定

#### 設定ファイル (`config`)
```bash
# プロトコル設定 (BOOT/REPORT)
PROTOCOL=REPORT

# デバイスID設定
DEVICEID=0
```

#### デバイスプロファイル例
```bash
# マウス設定ファイル (MOUSE-046d:c52b)
PROTOCOL REPORT
ReportID 02
BUTTON 1 0 16
X      3 0 12
Y      4 4 12
WHEEL  6 0 8
PAN    7 0 8
```

### Luaスクリプト例

#### 自動タイピング
```lua
-- 基本的なタイピング
type("Hello World!\n", 50)
```

#### キーボード制御
```lua
-- Ctrl+A (全選択)
keypress(0xE0)  -- Left Ctrl
keypress(0x04)  -- A
sleep(50)
keyrelease(0x04)
keyrelease(0xE0)

-- Alt+Tab
keypress(0xE2)  -- Left Alt
keypress(0x2B)  -- Tab
sleep(100)
keyrelease(0x2B)
keyrelease(0xE2)
```

### CDCコマンド

USB HID SwitcherはPCから仮想COMポート（CDC）として認識され、シリアル通信で各種コマンドを送信できます。
任意のターミナルソフト（PuTTY、TeraTermなど）やシリアル通信プログラムから接続可能です。

#### システム情報・管理コマンド
```bash
# バージョン情報表示
version

# 接続USBデバイス一覧表示
list

# コマンドキュー状態確認
queue
```

#### ファイルシステム操作
```bash
# ファイル・ディレクトリ一覧表示
ls

# ファイル内容表示
cat <filename>

# ファイル削除
rm <filename>
```

#### Luaスクリプト実行
```bash
# ファイルからLuaスクリプト実行
run <filename>

# 例：自動タイピングスクリプトの実行
run hello.lua
```

#### ファイル転送
```bash
# Base64エンコードファイルの受信
receive <filename>
# または短縮形
rcv <filename>

# プレーンテキストファイルの作成（Ctrl+Dで終了）
prog <filename>
```

#### キーボード言語設定
```bash
# 言語設定変更
lang <language>

# 現在の言語設定確認
lang

# 例：日本語キーボード設定
lang ja
```

**対応言語**: `us` (US English), `ja` (Japanese)

#### CDCコマンド使用例

**ファイル転送の例（Base64）:**
```bash
# ファイル受信開始
rcv hello.lua

# Base64データを送信（複数行可能）
dHlwZSgiSGVsbG8gV29ybGQhIiwgNTApCg==

# 空行で送信終了（Enterのみ）

```

**プレーンテキスト入力の例:**
```bash
# プログラムモード開始
prog autorun.lua

# 直接Luaコードを入力
type("Hello from USB HID Switcher!", 50)
sleep(1000)
keypress(0x28)  -- Enter key

# Ctrl+D で保存・終了
```

**デバイス情報確認の例:**
```bash
> list
Connected USB Host devices:
Addr Inst  VID:PID   Type       Status
-----------------------------------
  1    0   046D:C52B Mouse      Connected
  2    0   1234:5678 Keyboard   Connected
-----------------------------------
```

## プロジェクト構造

```
USB_HID_Switcher/
├── usb_hid_switcher/          # メインプロジェクト
│   ├── usb_hid_switcher.c     # メイン関数
│   ├── USBHostTask.c          # USB Host制御
│   ├── USBDeviceTask.c        # USB Device制御
│   ├── LuaTask.c              # Lua統合
│   ├── LEDtask.c              # LED制御
│   ├── OLEDtask.c             # OLED制御
│   ├── CDCCmd.c               # CDC コマンド処理
│   ├── fstask.c               # ファイルシステム
│   └── README_*.md            # 詳細ドキュメント
├── freertos/                  # FreeRTOS RTOS
├── lua/                       # Lua インタープリター
├── littlefs/                  # ファイルシステム
├── pico_pio_usb/              # PIO-USB ライブラリ
└── tinyusb/                   # USB スタック
```

## LED状態表示

| 色 | 状態 |
|---|---|
| 🟡 黄 | 別Picoへ信号転送中（UART経由出力） |
| 🟢 緑 | アクティブ |
| 点滅 | アクティブ状態（入力処理中） |

## 対応デバイス例

### マウス
- Logicool G300s (046d:c246)
- Logicool Unified Receiver (046d:c52b)
- 各種ゲーミングマウス

### キーボード
- 標準USBキーボード

## 技術仕様

- **CPU**: RP2040 (133MHz)
- **RAM**: 264KB / 520KB
- **Flash**: 2MB (ファイルシステム用に一部使用)
- **USB**: Full Speed (12Mbps)
- **RTOS**: FreeRTOS
- **File System**: LittleFS

## ライセンス

このプロジェクトはMITライセンスです。各コンポーネントのライセンスは各コンポーネントのライセンスファイルを参照してください。

## コントリビューション

バグ報告や機能提案はIssueでお願いします。プルリクエストも歓迎します。

## 関連リンク

- [Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB)
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
- [TinyUSB](https://github.com/hathach/tinyusb)
- [FreeRTOS](https://www.freertos.org/)
- [LittleFS](https://github.com/littlefs-project/littlefs)
- [Lua](https://www.lua.org/)