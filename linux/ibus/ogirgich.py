#!/usr/bin/env python3
"""Eski o'zbek yozuvini (lotin va kirill) yangi lotin alifbosiga o'giradi.

Yangi harflar Ö, Ğ, Ş va Ç (21.10.2026 qonuni, 22.04.2027 dan kuchga kiradi).
Ogirgich belgilarni birma-bir oladi: IBus dvigateli har tugmani, matn
o'giruvchi esa butun matnni shu bitta sinf orqali o'tkazadi.

    python3 ogirgich.py "O'zbekiston, Ўзбекистон"   →   Özbekiston, Özbekiston
"""
import sys

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


def harfmi(c):
    return bool(c) and c.isalpha() and c not in APOSTROF


class Ogirgich:
    def __init__(self, lotin=True, kirill=True, sh_ajrat=True, tutuq=False):
        self.lotin = lotin          # sh → ş, ch → ç, o' → ö, g' → ğ
        self.kirill = kirill        # kirill harflari → yangi lotin
        self.sh_ajrat = sh_ajrat    # s'h → sh (Is'hoq → Ishoq)
        self.tutuq = tutuq          # so'z ichidagi ' → ʼ
        self.kutish = ''            # hali qaror kutayotgan qism
        self.oldingi = ''           # oxirgi chiqqan belgi; '' — boshlanish, None — noma'lum
        self._yumshoq = False       # ь dan keyin: е → ye

    def qabul(self, c):
        """Bitta belgini oladi va tasdiqlangan matnni qaytaradi."""
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
        """Kutayotgan qism ekranda qanday ko'rinadi (tagiga chizilgan matn)."""
        k = self.kutish
        if k and k[0] == KATTA_KIRILL:
            return k[1:].capitalize()
        return k

    def orqaga(self):
        """Backspace: kutayotgan qismning oxirgi belgisini o'chiradi."""
        k = self.kutish
        self.kutish = '' if not k or k[0] == KATTA_KIRILL else k[:-1]

    def xom(self, c):
        """Belgini o'girmasdan chiqaradi (AltGr bilan yozilgan asl belgi)."""
        return self._chiqar(c)

    def unut(self):
        """Kursor boshqa joyga o'tdi: oldingi belgi noma'lum."""
        self.oldingi = None
        self._yumshoq = False

    # -- ichki qoidalar --

    def _chiqar(self, matn):
        if matn:
            self.oldingi = matn[-1]
            self._yumshoq = False
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


def ogir(matn, **sozlama):
    """Butun matnni yangi yozuvga o'giradi."""
    og = Ogirgich(**sozlama)
    return ''.join(og.qabul(c) for c in matn) + og.tugat()


if __name__ == '__main__':
    matn = ' '.join(sys.argv[1:]) if len(sys.argv) > 1 else sys.stdin.read()
    sys.stdout.write(ogir(matn) + ('\n' if len(sys.argv) > 1 else ''))
