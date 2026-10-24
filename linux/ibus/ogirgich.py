#!/usr/bin/env python3
"""Eski o'zbek yozuvini (lotin va kirill) yangi lotin alifbosiga o'giradi.

Yangi harflar Ö, Ğ, Ş va Ç (21.10.2026 qonuni, 22.04.2027 dan kuchga kiradi).
Ogirgich belgilarni birma-bir oladi: IBus dvigateli har tugmani, matn
o'giruvchi esa butun matnni shu bitta sinf orqali o'tkazadi.

Matn so'z (bo'shliqqacha bo'lgan bo'lak) bo'yicha kuzatiladi. Havola, email,
harf-raqamli so'z va o'zbekchada yo'q harfli so'z (w, yolg'iz c) o'girilmaydi;
xorijiy so'zlar lug'ati (Chrome, Shell, CHARGE) so'z tugaganda tiklanadi.

    python3 ogirgich.py "O'zbekiston, Ўзбекистон"   →   Özbekiston, Özbekiston
    python3 ogirgich.py < eski.txt > yangi.txt
"""
import re
import sys
from pathlib import Path

APOSTROF = frozenset("'`‘’ʻʼ")
TUTUQ = 'ʼ'                        # U+02BC
UNLI = frozenset('aeiouöAEIOUÖ')

KIRILL = {
    'а': 'a', 'б': 'b', 'в': 'v', 'г': 'g', 'д': 'd', 'ё': 'yo', 'ж': 'j',
    'з': 'z', 'и': 'i', 'й': 'y', 'к': 'k', 'л': 'l', 'м': 'm', 'н': 'n',
    'о': 'o', 'п': 'p', 'р': 'r', 'с': 's', 'т': 't', 'у': 'u', 'ф': 'f',
    'х': 'x', 'ч': 'ç', 'ш': 'ş', 'щ': 'ş', 'ы': 'i', 'э': 'e', 'ю': 'yu',
    'я': 'ya', 'ў': 'ö', 'қ': 'q', 'ғ': 'ğ', 'ҳ': 'h',
}
KIRILL_ORINGA_QARAB = frozenset('ецъь')

LOTIN_BOSH = frozenset('sScCoOgG')  # keyingi belgiga qarab o'zgarishi mumkin
HARF_H = {'s': 'ş', 'S': 'Ş', 'c': 'ç', 'C': 'Ç'}
HARF_APOSTROF = {'o': 'ö', 'O': 'Ö', 'g': 'ğ', 'G': 'Ğ'}
KATTA_KIRILL = '\x00'               # kutish belgisi: Ё → «Yo» yoki «YO» keyingi harfga qarab

# So'z shu ko'rinishda bo'lsa o'girilmaydi: havola, email, @nom, harf-raqamli (ch1),
# domen (shifo.uz) va o'zbekchada yo'q harf — w yoki h siz c (Schwann, Cochrane)
HAVOLA = re.compile(r'://|^www\.|@', re.I)
DOMEN = re.compile(r'[A-Za-z]\.[A-Za-z]{2,}')
HARF_RAQAM = re.compile(r'[A-Za-z]\d|\d[A-Za-z]')
XORIJIY = re.compile(r'[wW]|[cC](?=[^hH])')
NOTANISH = re.compile('[äöüßéèêëáàâïñøåæ]', re.I)       # eski o'zbek lotinida yo'q harf: Pöschl, Tröltsch
DIS_H = re.compile(r'(?:^|[^a-z])dish(?:ormon|idro|armon)', re.I)  # dis + gormon: s va h alohida
UNLI_HARF = re.compile('[AEIOUaeiou]')
QISM_AJRATGICH = re.compile('([-/–])')
TINISH = '.,;:!?"«»„“”()[]{}…—–*'
KIRILL_HARF = re.compile('[\u0400-\u04ff]')
# Tugmadan yozilgan yangi harf istisno qoidalarida eski yozuvdagidek ko'riladi: [ tugmasidagi ö
# NOTANISH (Pöschl) deb, W dagi ş esa XORIJIY deb topilmasin.
ESKI_SHAKL = {'ş': 'sh', 'Ş': 'SH', 'ç': 'ch', 'Ç': 'CH', 'ö': "o'", 'Ö': "O'", 'ğ': "g'", 'Ğ': "G'"}
LUGAT_FAYL = Path(__file__).with_name('lugat.txt')


def harfmi(c):
    return bool(c) and c.isalpha() and c not in APOSTROF


def havolami(soz):
    """Havola, email, @nom, domen, harf-raqamli yoki o'zbekchada yo'q harfli so'z."""
    return bool(HAVOLA.search(soz) or DOMEN.search(soz) or HARF_RAQAM.search(soz) or NOTANISH.search(soz))


def qisqartmami(soz):
    """Unlisiz katta harfli qisqartma (TSH, PTCH, CH-III ning CH qismi)."""
    for qism in re.split(r'[-/]', soz.strip(TINISH)):
        if re.fullmatch('[A-Z]{2,}', qism) and not UNLI_HARF.search(qism):
            return True
    return False


def xom_qolsinmi(soz, yakuniy=False):
    """So'z o'girilmay qolishi kerakmi (lug'atsiz qoidalar). yakuniy — so'z tugagan."""
    if havolami(soz) or XORIJIY.search(soz):
        return True
    if not yakuniy:
        return False
    return soz.rstrip(TINISH)[-1:] in ('c', 'C') or qisqartmami(soz) or bool(DIS_H.search(soz))


def _harf_holati(asl, namuna):
    """Lug'atdagi so'zga yozilgan so'zning harf holatini beradi (CHROME, Chrome, chrome)."""
    if len(namuna) > 1 and namuna.isupper():
        return asl.upper()
    if namuna[:1].isupper():
        return asl[:1].upper() + asl[1:]
    return asl[:1].lower() + asl[1:]


class Lugat:
    """Xorijiy so'zlar: o'girilgan ko'rinishi (Çrome) ham, asli (chrome) ham taniladi."""

    def __init__(self, sozlar=()):
        self.kalit = {}
        for soz in sozlar:
            self.qosh(soz)

    def qosh(self, soz):
        soz = soz.strip()
        if len(soz) < 2:
            return
        for k in (soz.lower(), sof_ogir(soz).lower()):
            self.kalit[k] = soz

    def __len__(self):
        return len({*self.kalit.values()})

    def tuzat(self, token):
        """Token ichidagi lug'at so'zini asliga qaytaradi; o'zgarmasa None."""
        bosh = len(token) - len(token.lstrip(TINISH))
        oxir = len(token.rstrip(TINISH))
        soz = token[bosh:oxir]
        if len(soz) < 2:
            return None
        yangi = self._top(soz)
        if yangi is None and '-' in soz:                    # Arnold-Chiari, four-channel
            qismlar = [self._top(q) or q for q in soz.split('-')]
            yangi = '-'.join(qismlar)
        if yangi is None or yangi == soz:
            return None
        return token[:bosh] + yangi + token[oxir:]

    def _top(self, soz):
        if len(soz) < 2:
            return None
        k = soz.lower()
        asl = self.kalit.get(k)
        if asl:
            return _harf_holati(asl, soz)
        # qo'shimchali shakl: Chromeda, Shellga (qisqa so'zda emas — bashorat ≠ bash+orat)
        for n in range(len(k) - 1, max(4, len(k) - 6), -1):
            asl = self.kalit.get(k[:n])
            if asl and n >= 5:
                return _harf_holati(asl, soz[:n]) + soz[n:]
        return None


def lugat_oqi(*fayllar):
    """Lug'at fayllaridan so'zlar (har qatorda bitta, # dan keyingisi izoh)."""
    sozlar = []
    for fayl in fayllar:
        try:
            for qator in Path(fayl).read_text(encoding='utf-8').splitlines():
                qator = qator.split('#', 1)[0].strip()
                if qator:
                    sozlar.append(qator)
        except OSError:
            pass
    return sozlar


class Ogirgich:
    def __init__(self, lotin=True, kirill=True, sh_ajrat=True, tutuq=False, istisnolar=True, lugat=None):
        self.lotin = lotin          # sh → ş, ch → ç, o' → ö, g' → ğ
        self.kirill = kirill        # kirill harflari → yangi lotin
        self.sh_ajrat = sh_ajrat    # s'h → sh (Is'hoq → Ishoq)
        self.tutuq = tutuq          # so'z ichidagi ' → ʼ
        self.istisnolar = istisnolar  # havola, email, xorijiy so'z o'girilmaydi
        self.lugat = lugat          # Lugat yoki None
        self.kutish = ''            # hali qaror kutayotgan qism
        self.oldingi = ''           # oxirgi chiqqan belgi; '' — boshlanish, None — noma'lum
        self._yumshoq = False       # ь dan keyin: е → ye
        self._token_yangi()

    # -- so'z (token) holati --

    def _token_yangi(self):
        self.token_xom = ''         # yozilgan belgilar (to'g'ri tugmadagi ş, ç ham shu yerda)
        self.token_tugma = ''       # tugmaning asl belgisi bilan (W tugmasidagi ş → w): www. ni tanish uchun
        self._tekshir = []          # har belgi istisno tekshiruvi uchun (tugmadagi ö → o'), ESKI_SHAKL
        self.token_chiq = ''        # tasdiqlangan natija (kutayotgan qismsiz)
        self.otkaz = False          # so'z o'girilmaydigan deb topildi
        self.buzildi = False        # so'z ichida o'chirildi — orqaga tuzatish qilinmaydi

    def ekran(self):
        """Joriy so'z ekranda qanday turishi kerak (kutayotgan qism bilan)."""
        return self.token_chiq + self.korinish()

    @property
    def token_tekshir(self):
        return ''.join(self._tekshir)

    def _asl_nusxa(self, yakuniy=False):
        """So'z o'girilmasligi kerak bo'lsa — uning asl ko'rinishi, aks holda None."""
        if xom_qolsinmi(self.token_tekshir, yakuniy):
            return self.token_xom
        if havolami(self.token_tugma):
            return self.token_tugma
        return None

    def yakunla(self):
        """So'z tugadi: kutayotgan qism chiqadi, istisno bo'lsa so'z asliga qaytadi."""
        self.tugat()
        if self.buzildi or not self.istisnolar:
            return
        xom = self.token_xom
        lotin_soz = not KIRILL_HARF.search(xom)
        butun = self.lugat.tuzat(self.token_chiq) if self.lugat and lotin_soz else None
        if butun:                                        # lug'atdagi butun nom: Shy-Drager, MACH-NC
            self.token_chiq = butun
            return
        if QISM_AJRATGICH.search(xom.strip(TINISH)) and not havolami(self.token_tugma) and lotin_soz:
            # C-uchli, MR-shartli, Arnold-Chiari: har qism alohida tekshiriladi
            self.token_chiq = ''.join(q if QISM_AJRATGICH.fullmatch(q) else self._qism(q, t)
                                      for q, t in zip(QISM_AJRATGICH.split(xom),
                                                      QISM_AJRATGICH.split(self.token_tekshir)))
            return
        if self.otkaz:
            return
        asl = self._asl_nusxa(yakuniy=True)
        if asl is not None:
            self.token_chiq = asl
        elif self.lugat and not KIRILL_HARF.search(self.token_xom):     # lug'at — lotincha yozilgan so'zga
            tuzatilgan = self.lugat.tuzat(self.token_chiq)
            if tuzatilgan:
                self.token_chiq = tuzatilgan

    def _qism(self, qism, tekshir):
        """Chiziqcha bilan ajralgan bo'lak: istisno bo'lsa asli, aks holda o'girilgani."""
        if not qism or xom_qolsinmi(tekshir, yakuniy=True):
            return qism
        og = Ogirgich(self.lotin, self.kirill, self.sh_ajrat, self.tutuq, istisnolar=False)
        chiq = ''.join(og.qabul(c) for c in qism) + og.tugat()
        tuzatilgan = self.lugat.tuzat(chiq) if self.lugat else None
        return tuzatilgan or chiq

    # -- belgilar --

    def qabul(self, c, tugma=None):
        """Bitta belgini oladi va tasdiqlangan matnni qaytaradi.

        tugma — to'g'ri tugmadagi yangi harfning asl belgisi (W tugmasidagi ş uchun 'w'):
        havola yoki email ichida shu asl belgi yoziladi.
        """
        if c.isspace():
            matn = self.tugat() + self._chiqar(c)
            self._token_yangi()
            return matn
        self.token_xom += c
        self.token_tugma += tugma or c
        self._tekshir.append(ESKI_SHAKL.get(c, c) if tugma else c)
        if self.istisnolar and not self.otkaz:
            asl = self._asl_nusxa()
            if asl is not None:
                self.otkaz = True
                matn = self.tugat() + self._chiqar(tugma or c)
                self.token_chiq = asl                  # so'z boshidan asliga qaytadi
                return matn
        if self.otkaz:
            return self._chiqar(tugma or c)
        if not self.kutish:
            return self._yangi(c)
        natija = self._davom(c)
        if natija is None:
            return self.tugat() + self._yangi(c)
        matn, yutdi = natija
        return matn if yutdi else matn + self._yangi(c)

    def tugat(self):
        """Kutayotgan qismni boricha chiqaradi (keyingi belgi kelmaydi)."""
        k, self.kutish = self.kutish, ''
        if not k:
            return ''
        if k[0] == KATTA_KIRILL:
            return self._chiqar(k[1:].capitalize())
        return self._chiqar(k)

    def korinish(self):
        """Kutayotgan qism ekranda qanday ko'rinadi."""
        k = self.kutish
        if k and k[0] == KATTA_KIRILL:
            return k[1:].capitalize()
        return k

    def orqaga(self):
        """Backspace kutayotgan qismda: uning oxirgi belgisi o'chadi."""
        k = self.kutish
        self.kutish = '' if not k or k[0] == KATTA_KIRILL else k[:-1]
        self.token_xom = self.token_xom[:-1]
        self._tekshir = self._tekshir[:-1]

    def ochirildi(self):
        """Backspace tasdiqlangan matnda (ilova o'zi o'chirdi): so'z endi aniq kuzatilmaydi."""
        self.token_chiq = self.token_chiq[:-1]
        self.token_xom = self.token_xom[:-1]
        self._tekshir = self._tekshir[:-1]
        self.buzildi = True

    def xom(self, c):
        """Belgini o'girmasdan chiqaradi (AltGr bilan yozilgan asl belgi)."""
        matn = self.tugat()
        self.token_xom += c
        self.token_tugma += c
        self._tekshir.append(c)
        matn += self._chiqar(c)
        if self.istisnolar and not self.otkaz and xom_qolsinmi(self.token_tekshir):
            self.otkaz = True
            self.token_chiq = self.token_xom
        return matn

    def unut(self):
        """Kursor boshqa joyga o'tdi: oldingi belgi va joriy so'z noma'lum."""
        self.kutish = ''
        self.oldingi = None
        self._yumshoq = False
        self._token_yangi()

    # -- ichki qoidalar --

    def _chiqar(self, matn):
        if matn:
            self.oldingi = matn[-1]
            self._yumshoq = False
            self.token_chiq += matn
        return matn

    def _yangi(self, c):
        if self.lotin and c in LOTIN_BOSH:
            self.kutish = c
            return ''
        if self.tutuq and c in "'’" and harfmi(self.oldingi):
            self.kutish = "'"
            return ''
        if self.kirill:
            past = c.lower()
            if past in KIRILL or past in KIRILL_ORINGA_QARAB:
                return self._kirill(c, past)
        return self._chiqar(c)

    def _davom(self, c):
        """Kutayotgan qismga c qo'shiladi. Qaytaradi: (matn, c yutildimi) yoki None."""
        k = self.kutish
        if k[0] == KATTA_KIRILL:
            self.kutish = ''
            lat = k[1:]
            return self._chiqar(lat.upper() if c.isupper() else lat.capitalize()), False
        if k == "'":
            self.kutish = ''
            return self._chiqar(TUTUQ if harfmi(c) else "'"), False
        if k in HARF_H and c in 'hH':
            self.kutish = ''
            return self._chiqar(HARF_H[k]), True
        if k in ('s', 'S') and c in APOSTROF and self.sh_ajrat:
            self.kutish = k + c
            return '', True
        if len(k) == 2:                     # s' dan keyin
            self.kutish = ''
            if c in 'hH':
                return self._chiqar(k[0] + c), True
            apostrof = TUTUQ if self.tutuq and harfmi(c) else k[1]
            return self._chiqar(k[0] + apostrof), False
        if k in HARF_APOSTROF and c in APOSTROF:
            self.kutish = ''
            return self._chiqar(HARF_APOSTROF[k]), True
        return None

    def _kirill(self, c, past):
        if past == 'ь':
            self._yumshoq = True
            return ''
        if past == 'ъ':
            return self._chiqar(TUTUQ if self.tutuq else "'")
        if past == 'е':
            o = self.oldingi
            lat = 'ye' if self._yumshoq or not harfmi(o) or o in UNLI else 'e'
        elif past == 'ц':
            lat = 'ts' if (self.oldingi or '') in UNLI else 's'
        else:
            lat = KIRILL[past]
        if c == past:
            return self._chiqar(lat)
        if len(lat) == 1 or (harfmi(self.oldingi) and self.oldingi.isupper()):
            return self._chiqar(lat.upper())
        self.kutish = KATTA_KIRILL + lat
        return ''


ESKI_HARF = {'ş': 'sh', 'ç': 'ch', 'ö': "o'", 'ğ': "g'"}


def eski_yozuvda(matn):
    """Yangi harflar eski yozuvga: ş → sh, Ç → Ch (keyingi harf katta bo'lsa CH)."""
    natija = []
    for i, c in enumerate(matn):
        past = c.lower()
        if past not in ESKI_HARF:
            natija.append(c)
            continue
        eski = ESKI_HARF[past]
        if c != past:
            eski = eski.upper() if matn[i + 1:i + 2].isupper() else eski[0].upper() + eski[1:]
        natija.append(eski)
    return ''.join(natija)


def sof_ogir(matn):
    """Istisnosiz, toza qoidalar bilan o'giradi (lug'at kalitlari uchun)."""
    og = Ogirgich(istisnolar=False)
    return ''.join(og.qabul(c) for c in matn) + og.tugat()


def ogir(matn, **sozlama):
    """Butun matnni yangi yozuvga o'giradi (so'zma-so'z, istisnolar bilan)."""
    og = Ogirgich(**sozlama)
    qismlar = []
    for c in matn:
        if c.isspace():
            og.yakunla()
            qismlar.append(og.token_chiq)
            og.qabul(c)
            qismlar.append(c)
        else:
            og.qabul(c)
    og.yakunla()
    qismlar.append(og.token_chiq)
    return ''.join(qismlar)


if __name__ == '__main__':
    lugat = Lugat(lugat_oqi(LUGAT_FAYL))
    matn = ' '.join(sys.argv[1:]) if len(sys.argv) > 1 else sys.stdin.read()
    sys.stdout.write(ogir(matn, lugat=lugat) + ('\n' if len(sys.argv) > 1 else ''))
