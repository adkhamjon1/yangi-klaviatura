#!/usr/bin/env python3
"""O'girish qoidalari sinovi:  python3 -m unittest -v linux/ibus/test_ogirgich.py"""
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ogirgich import Ogirgich, ogir  # noqa: E402


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
