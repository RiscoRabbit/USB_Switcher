# OLED Text Display Functions

このプロジェクトに座標を指定してOLEDに文字を表示する関数群を追加しました。

## 追加された関数

### 1. `oled_display_text(x, y, scale, text)`
指定した座標に文字を表示します。

**パラメータ:**
- `x`: X座標 (0-127、128ピクセル幅ディスプレイの場合)
- `y`: Y座標 (0-31、32ピクセル高ディスプレイの場合)  
- `scale`: フォントサイズの倍率 (1=通常サイズ、2=2倍サイズ等)
- `text`: 表示するテキスト文字列

**使用例:**
```c
oled_display_text(0, 0, 1, "Hello World!");     // 左上角に通常サイズで表示
oled_display_text(10, 16, 2, "BIG TEXT");       // (10,16)に2倍サイズで表示
```

### 2. `oled_display_text_centered(y, scale, text)`
指定したY座標で水平方向に中央揃えしてテキストを表示します。

**パラメータ:**
- `y`: Y座標 (0-31)
- `scale`: フォントサイズの倍率
- `text`: 表示するテキスト文字列

**使用例:**
```c
oled_display_text_centered(8, 1, "Centered Text");   // Y=8の位置に中央揃えで表示
oled_display_text_centered(20, 2, "BIG CENTERED");   // Y=20に2倍サイズで中央揃え
```

### 3. `oled_clear_display()`
OLEDディスプレイのバッファをクリアします。

**使用例:**
```c
oled_clear_display();  // ディスプレイをクリア
```

### 4. `oled_update_display()`
バッファの内容をOLEDディスプレイに反映します。

**使用例:**
```c
oled_update_display();  // バッファの内容を画面に表示
```

## 基本的な使用パターン

### パターン1: 単純なテキスト表示
```c
// ディスプレイをクリアして新しいテキストを表示
oled_clear_display();
oled_display_text(0, 0, 1, "Line 1");
oled_display_text(0, 8, 1, "Line 2");
oled_update_display();
```

### パターン2: 中央揃えテキスト
```c
// 中央揃えでテキストを表示
oled_clear_display();
oled_display_text_centered(4, 1, "Title");
oled_display_text_centered(12, 1, "Subtitle");
oled_update_display();
```

### パターン3: 動的な情報表示
```c
// カウンターやセンサー値の表示
char buffer[32];
snprintf(buffer, sizeof(buffer), "Count: %d", counter_value);

oled_clear_display();
oled_display_text_centered(8, 2, buffer);
oled_update_display();
```

## 座標系について

- X座標: 0-127 (128ピクセル幅)
- Y座標: 0-31 (32ピクセル高)
- 座標原点(0,0)は左上角
- フォントサイズ: scale=1で約8×8ピクセル、scale=2で約16×16ピクセル

## 使用例

詳細な使用例は `oled_text_example.c` と `oled_text_example.h` を参照してください。これらのファイルには以下が含まれています:

- `oled_text_display_example()`: 全機能のデモンストレーション
- `display_message_at()`: 簡単なメッセージ表示
- `display_centered_message()`: 簡単な中央揃えメッセージ表示

## 注意事項

1. OLED関数を使用する前に、`isOLEDPresent()`でOLEDデバイスが接続されているかを確認してください
2. 複数のテキストを表示する場合は、最後に`oled_update_display()`を呼び出してください
3. 座標がディスプレイの範囲を超えないよう注意してください
4. テキストが長すぎる場合、ディスプレイの右端で切り取られます

## ファイル構成

- `OLEDtask.h`: 新しい関数の宣言を追加
- `OLEDtask.c`: 新しい関数の実装を追加
- `oled_text_example.h`: 使用例関数の宣言
- `oled_text_example.c`: 詳細な使用例とデモ関数