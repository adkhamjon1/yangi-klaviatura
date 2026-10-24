#!/usr/bin/env python3
"""Foydalanuvchi sozlash (GNOME): til ro'yxatidagi rejimlar va sozlamalar oynasi tugmasi.

    foydalanuvchi.py yoq      — «öz», «o'z», «ўз» (sozlamaga ko'ra) va Super+K
    foydalanuvchi.py sxema    — faqat W/C/[/] sxemasi (IBus'siz o'rnatish uchun)
    foydalanuvchi.py ochir    — hammasini olib tashlaydi
Sozlamalar oynasi ham shu moduldan foydalanadi.
"""
import os
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from gi.repository import Gio, GLib  # noqa: E402

import sozlama  # noqa: E402

PAPKA = os.path.dirname(os.path.abspath(__file__))
MANBALAR = 'org.gnome.desktop.input-sources'
TARTIB = ['ozbek-lotin', 'ozbek-eski', 'ozbek-kirill']        # til ro'yxatidagi o'rni
LOTIN, ESKI, KIRILL, SXEMA = (('ibus', 'ozbek-lotin'), ('ibus', 'ozbek-eski'),
                              ('ibus', 'ozbek-kirill'), ('xkb', 'ozbek'))

ROYXAT = 'org.gnome.settings-daemon.plugins.media-keys'
BITTA = 'org.gnome.settings-daemon.plugins.media-keys.custom-keybinding'
YOL = '/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/yangi-klaviatura/'
TUGMA = '<Super>k'
TEKSHIRILADI = ['org.gnome.desktop.wm.keybindings', 'org.gnome.shell.keybindings',
                'org.gnome.mutter.keybindings', 'org.gnome.mutter.wayland.keybindings', ROYXAT]


def bormi(sxema):
    manba = Gio.SettingsSchemaSource.get_default()
    return bool(manba and manba.lookup(sxema, True))


# -- til ro'yxati --

def manbalar():
    if not bormi(MANBALAR):
        return []
    return list(Gio.Settings.new(MANBALAR).get_value('sources').unpack())


def manba(juft, yoq):
    """Rejimni til ro'yxatiga qo'shadi (o'z tartibida) yoki undan oladi."""
    if not bormi(MANBALAR):
        return False
    royxat = manbalar() or [('xkb', 'us')]
    if yoq and juft not in royxat:
        keyingi = [i for i, (_, nom) in enumerate(royxat)
                   if nom in TARTIB and juft[1] in TARTIB and TARTIB.index(nom) > TARTIB.index(juft[1])]
        royxat.insert(keyingi[0] if keyingi else len(royxat), juft)
    elif not yoq and juft in royxat:
        royxat.remove(juft)
    else:
        return False
    Gio.Settings.new(MANBALAR).set_value('sources', GLib.Variant('a(ss)', royxat or [('xkb', 'us')]))
    Gio.Settings.sync()
    return True


def yoqilganmi():
    return LOTIN in manbalar()


def rejimlar_yoq():
    s = sozlama.oqi()
    manba(SXEMA, False)
    manba(LOTIN, True)
    manba(ESKI, s['eski'])
    manba(KIRILL, s['kirill_rejim'])


def rejimlar_ochir():
    for juft in (SXEMA, LOTIN, ESKI, KIRILL):
        manba(juft, False)


# -- tezkor tugma --

def band(tugma):
    """Tugma boshqa vazifaga berilgan bo'lsa, o'sha vazifa nomini qaytaradi."""
    t = tugma.lower()
    for sxema in filter(bormi, TEKSHIRILADI):
        sozlamalar = Gio.Settings.new(sxema)
        for kalit in sozlamalar.props.settings_schema.list_keys():
            qiymat = sozlamalar.get_value(kalit).unpack()
            if any(isinstance(q, str) and q.lower() == t for q in (qiymat if isinstance(qiymat, list) else [qiymat])):
                return f'{sxema} {kalit}'
    for yol in Gio.Settings.new(ROYXAT).get_strv('custom-keybindings'):
        if yol != YOL and Gio.Settings.new_with_path(BITTA, yol).get_string('binding').lower() == t:
            return yol
    return None


def tezkor_qosh(tugma=TUGMA):
    """Sozlamalar oynasi tugmasi; band bo'lsa qo'yilmaydi. Qaytaradi: (qo'yildimi, izoh)."""
    if not bormi(ROYXAT):
        return False, 'GNOME topilmadi'
    egasi = band(tugma)
    if egasi:
        return False, f'{tugma} band: {egasi}'
    bitta = Gio.Settings.new_with_path(BITTA, YOL)
    bitta.set_string('name', 'Özbek klaviaturasi')
    bitta.set_string('command', f'/usr/bin/python3 {PAPKA}/sozlamalar.py')
    bitta.set_string('binding', tugma)
    royxat = Gio.Settings.new(ROYXAT)
    yollar = royxat.get_strv('custom-keybindings')
    if YOL not in yollar:
        royxat.set_strv('custom-keybindings', yollar + [YOL])
    Gio.Settings.sync()
    return True, tugma


def tezkor_ochir():
    if not bormi(ROYXAT):
        return
    royxat = Gio.Settings.new(ROYXAT)
    royxat.set_strv('custom-keybindings', [y for y in royxat.get_strv('custom-keybindings') if y != YOL])
    bitta = Gio.Settings.new_with_path(BITTA, YOL)
    for kalit in ('name', 'command', 'binding'):
        bitta.reset(kalit)
    Gio.Settings.sync()


# -- IBus --

def ibus_qayta():
    """Yangi o'rnatilgan dvigatelni IBus ko'rishi uchun; XIM ko'prigi yopilgan bo'lsa qayta ochiladi."""
    if not shutil.which('ibus'):
        return
    subprocess.run(['ibus', 'write-cache'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(['ibus', 'restart'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    x11 = '/usr/libexec/ibus-x11'
    if os.environ.get('DISPLAY') and os.path.exists(x11) \
            and subprocess.run(['pgrep', '-x', 'ibus-x11'], stdout=subprocess.DEVNULL).returncode:
        subprocess.Popen([x11], start_new_session=True, stdin=subprocess.DEVNULL,
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


if __name__ == '__main__':
    buyruq = sys.argv[1] if len(sys.argv) > 1 else ''
    if buyruq == 'yoq':
        rejimlar_yoq()
        qoyildi, izoh = tezkor_qosh()
        print("✓ til ro'yxati:", ', '.join(nom for _, nom in manbalar()))
        print(f'✓ tezkor tugma: {izoh}' if qoyildi else f"! tezkor tugma qo'yilmadi — {izoh}")
    elif buyruq == 'sxema':
        manba(SXEMA, True)
        print("✓ til ro'yxati:", ', '.join(nom for _, nom in manbalar()))
    elif buyruq == 'ochir':
        rejimlar_ochir()
        tezkor_ochir()
        print("✓ til ro'yxatidan va tezkor tugmadan olib tashlandi")
    else:
        sys.exit(__doc__)
