/*
 * KanjiLCD1602 - LCD1602(HD44780系)に漢字・ひらがな・カタカナを表示する Arduino ライブラリ
 *
 * 東雲フォント16x16を 3x2マス(15x16ドット)に縮小し、CGRAM 0〜5 に登録して表示する。
 * 同時に表示できる全角文字は1文字(CGRAMは8個しかないため)。CGRAM 6,7 は自由に使える。
 *
 * LiquidCrystal / LiquidCrystal_I2C / hd44780 など、
 * createChar(), setCursor(), write() を持つ LCD クラスなら何でも使える。
 *
 * 収録文字の切り替え(このヘッダを #include する前に定義):
 *   (なし)                  compact : ひらがな・カタカナ・頻出漢字+南北線 (約461字 / 約14KB)
 *   #define KANJILCD_FONT_NAMBOKU   南北線のアナウンス文の文字だけ (142字 / 約4.4KB)
 *   #define KANJILCD_FONT_FULL      JIS第一水準を含む 約3,150字 (約98KB。ESP32/RP2040/ARM系ボード向け。AVRは不可)
 */
#pragma once
#include <Arduino.h>

#if defined(KANJILCD_FONT_FULL) && defined(__AVR__)
#error "KANJILCD_FONT_FULL は約98KBあり、AVR(Uno/Nano/Mega)のフラッシュ(64KB超のアドレス)では扱えません。ESP32/RP2040/ARM系ボードで使うか、既定(compact)または KANJILCD_FONT_NAMBOKU を使ってください。"
#elif defined(KANJILCD_FONT_FULL)
#include "kanjifont_full.h"
#elif defined(KANJILCD_FONT_NAMBOKU)
#include "kanjifont_namboku.h"
#else
#include "kanjifont_compact.h"
#endif

template <class LCD>
class KanjiLCD {
public:
  explicit KanjiLCD(LCD &lcd) : _lcd(lcd) {}

  // UTF-8文字列から1文字ぶんのUnicodeコードを取り出し、ポインタを進める。
  // 終端なら 0 を返す。BMP外の文字は 0xFFFD。
  static uint16_t nextCode(const char *&p) {
    uint8_t c = (uint8_t)*p;
    if (c == 0) return 0;
    if (c < 0x80) { p += 1; return c; }
    if ((c & 0xE0) == 0xC0 && p[1]) {
      uint16_t u = ((c & 0x1F) << 6) | (p[1] & 0x3F);
      p += 2; return u;
    }
    if ((c & 0xF0) == 0xE0 && p[1] && p[2]) {
      uint16_t u = ((uint16_t)(c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
      p += 3; return u;
    }
    p += 1;                         // 4バイト文字などは読み飛ばす
    while (((uint8_t)*p & 0xC0) == 0x80) p++;
    return 0xFFFD;
  }

  // 収録されているか
  static bool has(uint16_t code) { return findIndex(code) >= 0; }
  static bool has(const char *utf8) { const char *p = utf8; return has(nextCode(p)); }

  // Unicodeコードで表示。row は上段の行(0 または 2 など)。col は左端の桁。
  bool showCode(uint16_t code, uint8_t col, uint8_t row = 0) {
    int idx = findIndex(code);
    if (idx < 0) return false;
    writeGlyph((uint16_t)idx);
    _lcd.setCursor(col, row);
    for (uint8_t i = 0; i < 3; i++) _lcd.write((uint8_t)i);
    _lcd.setCursor(col, row + 1);
    for (uint8_t i = 3; i < 6; i++) _lcd.write((uint8_t)i);
    return true;
  }

  // UTF-8文字列の先頭1文字を表示。例: show("仙", 0)
  bool show(const char *utf8, uint8_t col, uint8_t row = 0) {
    const char *p = utf8;
    return showCode(nextCode(p), col, row);
  }

  // UTF-8文字列を1文字ずつ ms ミリ秒おきに切り替えて表示。例: showSequence("泉中央", 0, 800)
  void showSequence(const char *utf8, uint8_t col, uint16_t ms = 800, uint8_t row = 0) {
    const char *p = utf8;
    for (;;) {
      uint16_t code = nextCode(p);
      if (code == 0) break;
      if (showCode(code, col, row)) delay(ms);
    }
  }

private:
  LCD &_lcd;

  // INDEX(昇順)を二分探索
  static int findIndex(uint16_t code) {
    int lo = 0, hi = KANJILCD_GLYPH_COUNT - 1;
    while (lo <= hi) {
      int mid = (lo + hi) >> 1;
      uint16_t v = pgm_read_word(&KANJILCD_INDEX[mid]);
      if (v == code) return mid;
      if (v < code) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
  }

  // 30バイト(15x16, MSBから行優先)を 3x2マス = CGRAM 0〜5 に展開して登録
  void writeGlyph(uint16_t idx) {
    const uint8_t *g = &KANJILCD_GLYPHS[(uint32_t)idx * 30];
    for (uint8_t c = 0; c < 6; c++) {
      uint8_t cx = c % 3, ry = c / 3;
      uint8_t cell[8];
      for (uint8_t y = 0; y < 8; y++) {
        uint16_t pos = (uint16_t)(ry * 8 + y) * 15 + cx * 5;
        uint8_t b = pos >> 3;
        uint16_t v = (uint16_t)pgm_read_byte(g + b) << 8;
        if (b + 1 < 30) v |= pgm_read_byte(g + b + 1);
        cell[y] = (v >> (11 - (pos & 7))) & 31;
      }
      _lcd.createChar(c, cell);
    }
  }
};
