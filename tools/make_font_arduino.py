#!/usr/bin/env python3
"""東雲フォント(BDF)から Arduino 用ヘッダ(src/kanjifont_*.h)を生成する。

使い方:
    python tools/make_font_arduino.py tools/shnmk16.bdf

3種類を作る:
  kanjifont_namboku.h : tools/namboku.txt の文字だけ(約4.5KB・Uno向け)
  kanjifont_compact.h : ひらがな・カタカナ・頻出漢字+南北線の文字(約15KB・Uno向け)
  kanjifont_full.h    : JIS第一水準+南北線の文字(約100KB・Mega/ESP32/RP2040など向け)
各文字は15x16ドット(3x2マス)、1文字30バイト(1bit/dot)。
"""
import sys

W = 15  # 3マス x 5ドット
BLOCK = 128  # 1ブロックのグリフ数

LITE_KANJI = (
    "日一国会人年大十二本中長出三同時政事自行社見月分議後前民生連五発間対上部東者党地合市業内相方四定今回新場金員九入選立開手米力学問高代明実円関決子動京全目表戦経通外最言氏現理調体化田当八六約主題下首意法不来作性的要用制治度務強気小七成期公持野協取都和統以機平総加山思家話世受区領多県続進正安設保改数記院女初北午指権心界支第産結百派点教報済書府活原先共得解名交資予川向際査勝面委告軍文反元重近千考判認画海参売利組知案道信策集在件団別物側任引使求所次水半品昨論計死官増係感情投示変打直両式確果容必演歳争談能位置流格疑過局放常状球職与供役構割費付由説難優夫収断石違消神番規術備宅害配警育席訪乗残想声念助労例然限追商葉伝働形景落好退頭負渡失差末守若種美命福望非観察段横深申様財港識呼達良阪候程満敗値突光路科積他処太客否師登易速存飛殺号単座破年運独")


COMPACT_KANJI = (
    "年月日時分秒曜今午前後朝昼夜週毎"
    "一二三四五六七八九十百千万円零"
    "火水木金土"
    "天気晴雨雪曇風雲光温度湿圧"
    "東西南北上下左右中内外"
    "大小多少高低長短新古明暗強弱早遅"
    "人子女男私僕君名"
    "見聞言話読書行来出入開閉立座食飲寝起歩走止始終使作持待"
    "電池音楽計画面表示設定決力残量充放"
    "仙台宮城県市町村京都阪神奈川山海島"
    "本学校先生仕事会社家国世界"
    "春夏秋冬花鳥犬猫魚車駅道店米茶酒薬手足目口耳頭体心愛夢空星石林森田"
    "現在昨注意警告危険安全正誤成功失敗完了停止実行待機接続通信送受返変換選択確認")


def load_bdf(path):
    glyphs, cur, bits, inb = {}, None, [], False
    for line in open(path, encoding="latin-1"):
        t = line.split()
        if not t:
            continue
        if t[0] == "ENCODING":
            cur = int(t[1])
        elif t[0] == "BITMAP":
            bits, inb = [], True
        elif t[0] == "ENDCHAR":
            inb = False
            glyphs[cur] = bits
        elif inb:
            bits.append(int(t[0], 16))
    return glyphs


def pack(rows):
    """16x16 → 15x16 に面積平均で縮小し、MSBから行優先で30バイトに詰める。"""
    bits = []
    for r in rows:
        px = [(r >> (15 - i)) & 1 for i in range(16)]
        for x in range(W):
            a, b = x * 16 / W, (x + 1) * 16 / W
            s = sum(px[i] * (min(b, i + 1) - max(a, i))
                    for i in range(int(a), min(16, int(b) + 1)))
            bits.append(1 if s >= 0.5 * (b - a) else 0)
    out = bytearray(30)
    for i, v in enumerate(bits):
        if v:
            out[i >> 3] |= 0x80 >> (i & 7)
    return bytes(out)


def jis_of(ch):
    b = ch.encode("iso2022_jp")
    return (b[3] << 8) | b[4]


def kanji_rows(first, last):
    for row in range(first, last + 1):
        for col in range(0x21, 0x7F):
            try:
                yield bytes([row | 0x80, col | 0x80]).decode("euc_jp")
            except UnicodeDecodeError:
                pass



def build(path, chars):
    glyphs = load_bdf(path)
    table = {}
    for ch in chars:
        code = ord(ch)
        if code > 0xFFFF or code in table:
            continue
        try:
            j = jis_of(ch)
        except UnicodeEncodeError:
            continue
        if j in glyphs:
            table[code] = pack(glyphs[j])
    return table


def write_header(name, table, title):
    codes = sorted(table)
    with open("src/kanjifont_%s.h" % name, "w", encoding="utf-8") as f:
        f.write("// 自動生成: tools/make_font_arduino.py (東雲フォント 16x16 gothic / パブリックドメイン)\n")
        f.write("// %s  文字数: %d\n#pragma once\n#include <Arduino.h>\n\n" % (title, len(codes)))
        f.write("#define KANJILCD_GLYPH_COUNT %d\n\n" % len(codes))
        f.write("static const uint16_t KANJILCD_INDEX[] PROGMEM = {\n")
        for i in range(0, len(codes), 12):
            f.write("  " + ", ".join("0x%04X" % c for c in codes[i:i + 12]) + ",\n")
        f.write("};\n\nstatic const uint8_t KANJILCD_GLYPHS[] PROGMEM = {\n")
        for c in codes:
            f.write("  " + ",".join("0x%02X" % b for b in table[c]) + ",\n")
        f.write("};\n")
    print("%-8s %4d字  %.1f KB" % (name, len(codes), len(codes) * 32 / 1024))


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    path = sys.argv[1]
    text = open("tools/namboku.txt", encoding="utf-8").read()
    nan = [c for c in text if ord(c) >= 0x80 and not c.isspace()]

    base = [chr(c) for c in range(0x3041, 0x3094)] + [chr(c) for c in range(0x30A1, 0x30F7)]
    base += list("、。・？！ー「」")
    write_header("namboku", build(path, nan), "南北線のアナウンス文の文字のみ")
    write_header("compact", build(path, base + list(COMPACT_KANJI) + nan), "ひらがな・カタカナ・頻出漢字+南北線")
    full = base + [chr(c) for c in range(0xFF10, 0xFF1A)] + list(kanji_rows(0x30, 0x4F)) + nan
    write_header("full", build(path, full), "JIS第一水準+ひらがな・カタカナ+南北線")


main()
