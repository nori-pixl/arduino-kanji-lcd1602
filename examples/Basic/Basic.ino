// 基本の使い方: I2C接続のLCD1602に漢字を表示する
// ライブラリ「LiquidCrystal I2C」(Frank de Brabander版など)が必要
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <KanjiLCD.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);   // アドレスは 0x27 か 0x3F
KanjiLCD<LiquidCrystal_I2C> kanji(lcd);

void setup() {
  lcd.init();
  lcd.backlight();
}

void loop() {
  kanji.show("仙", 0);                 // 左端3マスに「仙」
  lcd.setCursor(4, 0);
  lcd.print("12/31");                 // 英数字は通常の文字で
  delay(2000);

  lcd.clear();
  kanji.showSequence("仙台のひらがなカタカナ漢字", 0, 800);  // 1文字ずつ切り替え
  lcd.clear();
}
