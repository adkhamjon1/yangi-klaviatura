#!/usr/bin/env python3
"""Windows ikonkalari (.ico): rejimlar «ÖZ», «O'Z», «ЎЗ» va ilova «Ö».

    python3 windows/ikonka.py   →  windows/tsf/ikonka/*.ico

Har o'lcham alohida chiziladi (16 pikselda ham harf aniq bo'lishi uchun).
"""
import subprocess
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

PAPKA = Path(__file__).resolve().parent / 'tsf' / 'ikonka'
OLCHAMLAR = [16, 20, 24, 32, 40, 48, 64, 256]
FON = (42, 98, 201, 255)       # ko'k
HARF = (255, 255, 255, 255)
IKONKALAR = {'oz': 'ÖZ', 'eski': "O'Z", 'kirill': 'ЎЗ', 'ilova': 'Ö'}


def shrift():
    yol = subprocess.run(['fc-match', '-f', '%{file}', 'DejaVu Sans:bold'], capture_output=True, text=True).stdout
    return yol or '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'


def chiz(matn, olcham, shrift_yoli):
    katta = olcham * 4                      # katta chizib kichraytiriladi — chetlar silliq
    rasm = Image.new('RGBA', (katta, katta), (0, 0, 0, 0))
    d = ImageDraw.Draw(rasm)
    d.rounded_rectangle([0, 0, katta - 1, katta - 1], radius=int(katta * 0.22), fill=FON)
    kenglik = katta * (0.86 if len(matn) > 1 else 0.7)
    hajm = katta
    while hajm > 4:
        f = ImageFont.truetype(shrift_yoli, hajm)
        chap, yuqori, ong, past = d.textbbox((0, 0), matn, font=f)
        if ong - chap <= kenglik and past - yuqori <= katta * 0.62:
            break
        hajm -= 2
    x = (katta - (ong - chap)) / 2 - chap
    y = (katta - (past - yuqori)) / 2 - yuqori
    d.text((x, y), matn, font=f, fill=HARF)
    return rasm.resize((olcham, olcham), Image.LANCZOS)


def main():
    PAPKA.mkdir(parents=True, exist_ok=True)
    yol = shrift()
    for nom, matn in IKONKALAR.items():
        rasmlar = [chiz(matn, o, yol) for o in OLCHAMLAR]
        rasmlar[-1].save(PAPKA / f'{nom}.ico', format='ICO', sizes=[(o, o) for o in OLCHAMLAR],
                         append_images=rasmlar[:-1])
        print(f'✓ {(PAPKA / f"{nom}.ico").relative_to(PAPKA.parent.parent.parent)}')


if __name__ == '__main__':
    main()
