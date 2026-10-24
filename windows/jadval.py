#!/usr/bin/env python3
"""Windows TSF xizmati uchun jadvallarni yasaydi (Linux'da, yangi-klaviatura ildizidan):

    python3 windows/jadval.py           — windows/tsf/jadval.inc va lugat.inc ni yozadi
    python3 windows/jadval.py --check   — fayllar manbaga mosligini tekshiradi

Manbalar: xarita.mjs (W/C/[/] tugmalari), /usr/share/X11/xkb/symbols/uz (o'zbek kirill
klaviaturasi — davlat standarti), linux/ibus/lugat.txt (xorijiy so'zlar).
"""
import re
import sys
from pathlib import Path

import gi

gi.require_version('IBus', '1.0')
from gi.repository import IBus  # noqa: E402

ILDIZ = Path(__file__).resolve().parent.parent
XKB_UZ = Path('/usr/share/X11/xkb/symbols/uz')

# XKB tugma nomi → (Windows skankodi, Windows VK) — US joylashuvi
TUGMALAR = {
    'TLDE': (0x29, 0xC0), 'AE01': (0x02, 0x31), 'AE02': (0x03, 0x32), 'AE03': (0x04, 0x33),
    'AE04': (0x05, 0x34), 'AE05': (0x06, 0x35), 'AE06': (0x07, 0x36), 'AE07': (0x08, 0x37),
    'AE08': (0x09, 0x38), 'AE09': (0x0A, 0x39), 'AE10': (0x0B, 0x30), 'AE11': (0x0C, 0xBD),
    'AE12': (0x0D, 0xBB), 'AD01': (0x10, 0x51), 'AD02': (0x11, 0x57), 'AD03': (0x12, 0x45),
    'AD04': (0x13, 0x52), 'AD05': (0x14, 0x54), 'AD06': (0x15, 0x59), 'AD07': (0x16, 0x55),
    'AD08': (0x17, 0x49), 'AD09': (0x18, 0x4F), 'AD10': (0x19, 0x50), 'AD11': (0x1A, 0xDB),
    'AD12': (0x1B, 0xDD), 'BKSL': (0x2B, 0xDC), 'AC01': (0x1E, 0x41), 'AC02': (0x1F, 0x53),
    'AC03': (0x20, 0x44), 'AC04': (0x21, 0x46), 'AC05': (0x22, 0x47), 'AC06': (0x23, 0x48),
    'AC07': (0x24, 0x4A), 'AC08': (0x25, 0x4B), 'AC09': (0x26, 0x4C), 'AC10': (0x27, 0xBA),
    'AC11': (0x28, 0xDE), 'AB01': (0x2C, 0x5A), 'AB02': (0x2D, 0x58), 'AB03': (0x2E, 0x43),
    'AB04': (0x2F, 0x56), 'AB05': (0x30, 0x42), 'AB06': (0x31, 0x4E), 'AB07': (0x32, 0x4D),
    'AB08': (0x33, 0xBC), 'AB09': (0x34, 0xBE), 'AB10': (0x35, 0xBF), 'LSGT': (0x56, 0xE2),
}


def belgi(nom):
    """XKB keysym nomi → Unicode belgi."""
    nom = nom.strip()
    if nom.startswith('U') and re.fullmatch('U[0-9A-Fa-f]{4,6}', nom):
        return chr(int(nom[1:], 16))
    kod = IBus.keyval_to_unicode(IBus.keyval_from_name(nom)).strip('\x00')
    if not kod:
        raise SystemExit(f'jadval.py: noma\'lum keysym {nom}')
    return kod


def c_belgi(c):
    return f"U'\\x{ord(c):04X}'" if c in "'\\" or not c.isprintable() else f"U'{c}'"


def xarita():
    matn = (ILDIZ / 'xarita.mjs').read_text(encoding='utf-8')
    return {k: b for k, *b in re.findall(r"(\w{4}): \['(.+?)', '(.+?)', '(.+?)', '(.+?)'\]", matn)}


def kirill_tugmalari():
    matn = XKB_UZ.read_text(encoding='utf-8')
    bolim = re.search(r'xkb_symbols "cyrillic" \{(.*?)\n\};', matn, re.S).group(1)
    jadval = {}
    for nom, oddiy, shift in re.findall(r'key <(\w{4})>\s*\{\[\s*([^,\]]+?)\s*,\s*([^,\]]+?)\s*\]\}', bolim):
        if nom in TUGMALAR:
            jadval[nom] = (belgi(oddiy), belgi(shift))
    return jadval


def jadval_inc():
    qatorlar = ['// windows/jadval.py yasagan — qo\'lda tahrirlanmaydi.', '#pragma once', '',
                '// «öz»: to\'g\'ri tugmalar (VK, oddiy, Shift, AltGr, AltGr+Shift)',
                'struct OzTugma { unsigned vk; char32_t oddiy, shift, altgr, altgr_shift; };',
                'static const OzTugma OZ_TUGMALAR[] = {']
    for nom, b in xarita().items():
        qatorlar.append(f'    {{0x{TUGMALAR[nom][1]:02X}, {", ".join(c_belgi(x) for x in b)}}},  // {nom}')
    qatorlar += ['};', '',
                 '// «ўз»: o\'zbek kirill klaviaturasi (skankod, oddiy, Shift) — davlat standarti, XKB «uz»',
                 'struct KirillTugma { unsigned skankod; char32_t oddiy, shift; };',
                 'static const KirillTugma KIRILL_TUGMALAR[] = {']
    for nom, (oddiy, shift) in sorted(kirill_tugmalari().items(), key=lambda x: TUGMALAR[x[0]][0]):
        qatorlar.append(f'    {{0x{TUGMALAR[nom][0]:02X}, {c_belgi(oddiy)}, {c_belgi(shift)}}},  // {nom}')
    qatorlar.append('};')
    return '\n'.join(qatorlar) + '\n'


def lugat_inc():
    lugat = (ILDIZ / 'linux/ibus/lugat.txt').read_text(encoding='utf-8')
    assert ')LUGAT"' not in lugat
    return ('// windows/jadval.py yasagan — linux/ibus/lugat.txt nusxasi.\n#pragma once\n\n'
            f'static const char ORNATILGAN_LUGAT[] = R"LUGAT({lugat})LUGAT";\n')


def main():
    tekshir = '--check' in sys.argv
    eskirgan = 0
    for yol, matn in ((ILDIZ / 'windows/tsf/jadval.inc', jadval_inc()), (ILDIZ / 'windows/tsf/lugat.inc', lugat_inc())):
        bor = yol.read_text(encoding='utf-8') if yol.exists() else None
        if bor == matn:
            continue
        if tekshir:
            print(f'✗ {yol.relative_to(ILDIZ)} eskirgan — python3 windows/jadval.py')
            eskirgan += 1
            continue
        yol.parent.mkdir(parents=True, exist_ok=True)
        yol.write_text(matn, encoding='utf-8')
        print(f'✓ {yol.relative_to(ILDIZ)}')
    if tekshir and not eskirgan:
        print('✓ jadvallar manbaga mos')
    sys.exit(1 if eskirgan else 0)


if __name__ == '__main__':
    main()
