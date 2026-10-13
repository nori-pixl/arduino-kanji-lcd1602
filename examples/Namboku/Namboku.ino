// 仙台市営地下鉄南北線の駅名を順に表示する
#define KANJILCD_FONT_NAMBOKU          // 南北線の文字だけ(約4.4KB)。Uno でも余裕
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <KanjiLCD.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
KanjiLCD<LiquidCrystal_I2C> kanji(lcd);

const char *STATIONS[] = {
  "富沢", "長町南", "長町", "長町一丁目", "河原町", "愛宕橋", "五橋", "仙台", "広瀬通",
  "勾当台公園", "北四番丁", "北仙台", "台原", "旭ヶ丘", "黒松", "八乙女", "泉中央"
};

void setup() {
  lcd.init();
  lcd.backlight();
}

void loop() {
  for (uint8_t i = 0; i < 17; i++) {
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("N");
    lcd.print(i + 1);                 // 駅番号(富沢が1)
    kanji.showSequence(STATIONS[i], 0, 700);
    delay(500);
  }
}
