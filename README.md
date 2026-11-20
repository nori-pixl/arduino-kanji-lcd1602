# KanjiLCD1602 (Arduino)

LCD1602(HD44780系)に**ひらがな・カタカナ・漢字**を表示する Arduino ライブラリです。
東雲フォント16x16を 3マス x 2段(15x16ドット)に縮小し、CGRAM 0〜5 に登録して表示します。

`LiquidCrystal` / `LiquidCrystal_I2C` / `hd44780` など、`createChar()`・`setCursor()`・`write()` を持つクラスなら使えます。

## 使い方

```cpp
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <KanjiLCD.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
KanjiLCD<LiquidCrystal_I2C> kanji(lcd);

void setup() { lcd.init(); lcd.backlight(); }

void loop() {
  kanji.show("日", 0);                         // 左端3マスに「日」
  kanji.showSequence("今日の曜日", 0, 800);  // 1文字ずつ切り替え
}
```

ソースファイルは **UTF-8** で保存してください

## API

| 関数 | 内容 |
|---|---|
| `show("日", col, row=0)` | UTF-8の先頭1文字を表示。収録外なら `false` |
| `showCode(0x4ED9, col, row=0)` | Unicodeコードで表示 |
| `showSequence("日曜", col, ms, row=0)` | 1文字ずつ ms ミリ秒おきに切り替え |
| `has("日")` | 収録されているか |

4行LCDなら `row` に 0 や 2 を指定できます。

## 制約

- 同時に表示できる全角文字は**1文字**です(CGRAMが8個しかなく、1文字で6個使うため)。CGRAM 6,7 は自由に使えます。
- 漢字を表示すると CGRAM 0〜5 が書き換わるので、自作文字を使うときは 6,7 を使ってください。
- 収録外の文字は何も表示しません(`false` を返します)。

## 文字を追加・作り直す

```bash
sudo apt install xfonts-shinonome pcf2bdf
gunzip -c /usr/share/fonts/X11/misc/shnmk16.pcf.gz > shnmk16.pcf
pcf2bdf shnmk16.pcf > tools/shnmk16.bdf
python tools/make_font_arduino.py tools/shnmk16.bdf
```

`tools/namboku.txt` の文字が南北線セットになります。別のテキストに替えれば、その文字のセットを作れます。


## ライセンス

MIT。フォントデータは東雲フォント(パブリックドメイン)由来です。
