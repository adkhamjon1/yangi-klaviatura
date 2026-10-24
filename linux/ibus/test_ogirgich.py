#!/usr/bin/env python3
"""O'girish qoidalari sinovi:  python3 -m unittest -v linux/ibus/test_ogirgich.py"""
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ogirgich import LUGAT_FAYL, Lugat, Ogirgich, eski_yozuvda, lugat_oqi, ogir  # noqa: E402


class Lotin(unittest.TestCase):
    def test_yangi_harflar(self):
        for eski, yangi in [
            ('shahar', 'şahar'), ('choy', 'çoy'), ("o'zbek", 'özbek'), ("g'isht", 'ğişt'),
            ("O'zbekiston", 'Özbekiston'), ('Toshkent', 'Toşkent'), ("so'ng", 'söng'),
            ("o'g'il", 'öğil'), ("bog'", 'boğ'), ('SHAHAR', 'ŞAHAR'), ('Chorvoq', 'Çorvoq'),
            ("to'g'ri", 'töğri'), ('kecha', 'keça'), ('osh', 'oş'),
        ]:
            self.assertEqual(ogir(eski), yangi, eski)

    def test_apostrof_turlari(self):
        for a in "'`‘’ʻʼ":
            self.assertEqual(ogir(f'o{a}zbek'), 'özbek', a)

    def test_tutuq_qoladi(self):
        self.assertEqual(ogir("ma'no"), "ma'no")
        self.assertEqual(ogir("mas'ul"), "mas'ul")
        self.assertEqual(ogir("san'at"), "san'at")

    def test_sh_ajratish(self):
        self.assertEqual(ogir("Is'hoq"), 'Ishoq')
        self.assertEqual(ogir("Is'hoq", sh_ajrat=False), "Is'hoq")

    def test_tutuq_belgisi(self):
        self.assertEqual(ogir("ma'no", tutuq=True), 'maʼno')
        self.assertEqual(ogir("mas'ul", tutuq=True), 'masʼul')
        self.assertEqual(ogir("'salom'", tutuq=True), "'salom'")

    def test_ochiq(self):
        self.assertEqual(ogir("shahar o'zbek", lotin=False), "shahar o'zbek")


class Kirill(unittest.TestCase):
    def test_sozlar(self):
        for eski, yangi in [
            ('Ўзбекистон', 'Özbekiston'), ('шаҳар', 'şahar'), ('ўғил', 'öğil'),
            ('Тошкент', 'Toşkent'), ('чой', 'çoy'), ('қишлоқ', 'qişloq'), ('ғўза', 'ğöza'),
            ('маъно', "ma'no"), ('мўъжиза', "mö'jiza"), ('юрак', 'yurak'), ('ёз', 'yoz'),
            ('Исҳоқ', 'Ishoq'),
        ]:
            self.assertEqual(ogir(eski), yangi, eski)

    def test_e(self):
        self.assertEqual(ogir('ер'), 'yer')
        self.assertEqual(ogir('поезд'), 'poyezd')
        self.assertEqual(ogir('кема'), 'kema')
        self.assertEqual(ogir('съезд'), "s'yezd")
        self.assertEqual(ogir('пьеса'), 'pyesa')
        self.assertEqual(ogir('Ер'), 'Yer')

    def test_ts(self):
        self.assertEqual(ogir('цирк'), 'sirk')
        self.assertEqual(ogir('лицей'), 'litsey')
        self.assertEqual(ogir('концерт'), 'konsert')

    def test_katta_harflar(self):
        self.assertEqual(ogir('Ёзги'), 'Yozgi')
        self.assertEqual(ogir('ЁЗ'), 'YOZ')
        self.assertEqual(ogir('ЮРАК'), 'YURAK')
        self.assertEqual(ogir('ПОЕЗД'), 'POYEZD')
        self.assertEqual(ogir('Я'), 'Ya')
        self.assertEqual(ogir('ШАҲАР'), 'ŞAHAR')

    def test_ochiq(self):
        self.assertEqual(ogir('шаҳар', kirill=False), 'шаҳар')

    def test_aralash(self):
        self.assertEqual(ogir("O'zbekiston — Ўзбекистон"), 'Özbekiston — Özbekiston')


class Istisnolar(unittest.TestCase):
    LUGAT = Lugat(lugat_oqi(LUGAT_FAYL))

    def test_havola_email_raqam(self):
        for matn in ['www.shifo.uz/chorak', 'https://choy.uz/shahar', 'info@choy.uz', '@shahar_uz',
                     'shifo.uz', 'ch1', 'sh2']:
            self.assertEqual(ogir(matn), matn, matn)
        self.assertEqual(ogir('2-chi sinf'), '2-çi sinf')
        self.assertEqual(ogir('#shahar'), '#şahar')

    def test_xorijiy_harf(self):
        for matn in ['Schwann', 'Cochrane', 'Washington', 'Sic']:
            self.assertEqual(ogir(matn), matn, matn)
        self.assertEqual(ogir('Schwann hujayrasi sharhi'), 'Schwann hujayrasi şarhi')

    def test_lugat(self):
        for matn, kutilgan in [
            ('Google Chrome va Shell', 'Google Chrome va Shell'),
            ('CHARGE sindromi', 'CHARGE sindromi'),
            ('Chromeda ochildi', 'Chromeda ochildi'.replace('ochildi', 'oçildi')),
            ('(Shell),', '(Shell),'),
            ('CHROME', 'CHROME'),
            ("O'Brien", "O'Brien"),
            ('Escherichia coli', 'Escherichia coli'),
        ]:
            self.assertEqual(ogir(matn, lugat=self.LUGAT), kutilgan, matn)
        self.assertEqual(ogir('Chrome'), 'Çrome')                    # lug'atsiz o'giriladi

    def test_lugat_qisqa_soz_qoshimchada_ishlamaydi(self):
        lugat = Lugat(['bash', 'push'])
        self.assertEqual(ogir('bash', lugat=lugat), 'bash')
        self.assertEqual(ogir('bashorat pushti', lugat=lugat), 'başorat puşti')

    def test_togri_tugma_bilan_yozilgan(self):
        """«öz» da C tugmasi ç beradi: Çrome ham lug'atdan taniladi."""
        og = Ogirgich(lugat=self.LUGAT)
        for c in 'Çrome':
            og.qabul(c)
        og.yakunla()
        self.assertEqual(og.token_chiq, 'Chrome')

    def test_lugat_kirillga_tegmaydi(self):
        lugat = Lugat(['shahar'])
        self.assertEqual(ogir('shahar шаҳар', lugat=lugat), 'shahar şahar')

    def test_korpus_qoidalari(self):
        for matn in ['Pöschl', 'Tröltsch', 'TSH', 'PTCH', 'CH-III', 'dishormonogenetik']:
            self.assertEqual(ogir(matn), matn, matn)
        self.assertEqual(ogir('AQSH SHAHAR'), 'AQŞ ŞAHAR')               # unlili katta harfli so'z o'giriladi
        self.assertEqual(ogir('Arnold-Chiari', lugat=self.LUGAT), 'Arnold-Chiari')
        self.assertEqual(ogir("og'izhalqum ToshPTI"), 'oğizhalqum ToşPTI')
        self.assertEqual(ogir('C-uchli MR-shartli EChT/CRB'), 'C-uçli MR-şartli EÇT/CRB')
        self.assertEqual(ogir('Bamford–Kowal–Bench Cheyne-Stokes', lugat=self.LUGAT), 'Bamford–Kowal–Bench Cheyne-Stokes')
        self.assertEqual(ogir("o'n-o'n besh"), 'ön-ön beş')
        self.assertEqual(ogir('Shy-Drager MACH-NC', lugat=self.LUGAT), 'Shy-Drager MACH-NC')

    def test_istisnolar_ochiq(self):
        self.assertEqual(ogir('www.shifo.uz', istisnolar=False), 'www.şifo.uz')

    def test_kirill_raqam(self):
        self.assertEqual(ogir('5А синф'), '5A sinf')

    def test_ekran_oqimi(self):
        """Dvigatel «darhol» rejimida ekranni og.ekran() ga tenglashtiradi."""
        og = Ogirgich()
        ekranlar = []
        for c in 'info@ch':
            og.qabul(c)
            ekranlar.append(og.ekran())
        self.assertEqual(ekranlar, ['i', 'in', 'inf', 'info', 'info@', 'info@c', 'info@ch'])
        og = Ogirgich()
        for c in 'shifo.uz':
            og.qabul(c)
        self.assertEqual(og.ekran(), 'shifo.uz')            # .uz kelganda so'z asliga qaytdi


HOLATLAR = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'sinov', 'holatlar.tsv')


def holatlar():
    """sinov/holatlar.tsv — C++ yadro ham shu holatlar bilan sinaladi."""
    with open(HOLATLAR, encoding='utf-8') as f:
        for qator in f:
            qator = qator.rstrip('\n')
            if qator and not qator.startswith('#'):
                bayroq, matn, kutilgan = qator.split('\t')
                yield bayroq, matn, kutilgan


def sozlama_bayroqdan(bayroq):
    s = {}
    for b in [] if bayroq == '-' else bayroq.split(','):
        if b == 'L':
            s['lugat'] = Lugat(lugat_oqi(LUGAT_FAYL))
        elif b.startswith('lugat='):
            s['lugat'] = Lugat(b[6:].split('|'))
        elif b == 'tutuq':
            s['tutuq'] = True
        else:
            s[{'-lotin': 'lotin', '-kirill': 'kirill', '-sh': 'sh_ajrat', '-istisno': 'istisnolar'}[b]] = False
    return s


class UmumiyHolatlar(unittest.TestCase):
    def test_holatlar(self):
        soni = 0
        for bayroq, matn, kutilgan in holatlar():
            with self.subTest(matn=matn, bayroq=bayroq):
                self.assertEqual(ogir(matn, **sozlama_bayroqdan(bayroq)), kutilgan)
            soni += 1
        self.assertGreater(soni, 90)


class EskiYozuvda(unittest.TestCase):
    def test_teskari(self):
        self.assertEqual(eski_yozuvda('Çrome'), 'Chrome')
        self.assertEqual(eski_yozuvda('ŞAHAR'), 'SHAHAR')
        self.assertEqual(eski_yozuvda('Özbekiston'), "O'zbekiston")
        self.assertEqual(eski_yozuvda('öğil'), "o'g'il")

    def test_altgr_kutishni_chiqaradi(self):
        og = Ogirgich()
        og.qabul('s')
        self.assertEqual(og.xom('w'), 'sw')
        self.assertEqual(og.ekran(), 'sw')


class Oqim(unittest.TestCase):
    """IBus dvigateli ko'radigan holat: tugma bosilgandagi javob."""

    def test_kutish_va_korinish(self):
        og = Ogirgich()
        self.assertEqual(og.qabul('s'), '')
        self.assertEqual(og.korinish(), 's')
        self.assertEqual(og.qabul('h'), 'ş')
        self.assertEqual(og.korinish(), '')

    def test_orqaga(self):
        og = Ogirgich()
        og.qabul('s')
        og.qabul("'")
        self.assertEqual(og.korinish(), "s'")
        og.orqaga()
        self.assertEqual(og.korinish(), 's')
        og.orqaga()
        self.assertEqual(og.korinish(), '')

    def test_nomalum_kontekst(self):
        og = Ogirgich()
        og.unut()
        self.assertEqual(og.qabul('е'), 'ye')
        og.oldingi = 'k'
        self.assertEqual(og.qabul('е'), 'e')


if __name__ == '__main__':
    unittest.main()
