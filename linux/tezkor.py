#!/usr/bin/env python3
"""Sozlamalar oynasini ochadigan tezkor tugma (GNOME):
    tezkor.py qosh [<Super>k]   — tugma band bo'lmasa qo'shadi
    tezkor.py olib-tashla
"""
import sys

from gi.repository import Gio

ROYXAT = 'org.gnome.settings-daemon.plugins.media-keys'
BITTA = 'org.gnome.settings-daemon.plugins.media-keys.custom-keybinding'
YOL = '/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/yangi-klaviatura/'
BUYRUQ = '/usr/bin/python3 /usr/local/share/yangi-klaviatura/sozlamalar.py'
TEKSHIRILADI = ['org.gnome.desktop.wm.keybindings', 'org.gnome.shell.keybindings',
                'org.gnome.mutter.keybindings', 'org.gnome.mutter.wayland.keybindings', ROYXAT]


def bormi(sxema):
    manba = Gio.SettingsSchemaSource.get_default()
    return bool(manba and manba.lookup(sxema, True))


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


def qosh(tugma):
    if not bormi(ROYXAT):
        print("GNOME topilmadi: tezkor tugma qo'yilmadi.")
        return
    egasi = band(tugma)
    if egasi:
        print(f"! {tugma} band ({egasi}) — tezkor tugma qo'yilmadi.")
        return
    bitta = Gio.Settings.new_with_path(BITTA, YOL)
    bitta.set_string('name', 'Özbek klaviaturasi')
    bitta.set_string('command', BUYRUQ)
    bitta.set_string('binding', tugma)
    royxat = Gio.Settings.new(ROYXAT)
    yollar = royxat.get_strv('custom-keybindings')
    if YOL not in yollar:
        royxat.set_strv('custom-keybindings', yollar + [YOL])
    Gio.Settings.sync()
    print(f'✓ tezkor tugma: {tugma} — sozlamalar oynasi')


def olib_tashla():
    if not bormi(ROYXAT):
        return
    royxat = Gio.Settings.new(ROYXAT)
    royxat.set_strv('custom-keybindings', [y for y in royxat.get_strv('custom-keybindings') if y != YOL])
    bitta = Gio.Settings.new_with_path(BITTA, YOL)
    for kalit in ('name', 'command', 'binding'):
        bitta.reset(kalit)
    Gio.Settings.sync()


if __name__ == '__main__':
    if sys.argv[1:2] == ['qosh']:
        qosh(sys.argv[2] if len(sys.argv) > 2 else '<Super>k')
    elif sys.argv[1:2] == ['olib-tashla']:
        olib_tashla()
    else:
        sys.exit(__doc__)
