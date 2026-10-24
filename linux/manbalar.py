#!/usr/bin/env python3
"""GNOME til ro'yxatini o'zgartiradi:  manbalar.py +ibus:ozbek-lotin -xkb:ozbek"""
import sys

from gi.repository import Gio, GLib

KALIT = 'org.gnome.desktop.input-sources'

manba = Gio.SettingsSchemaSource.get_default()
if not manba or not manba.lookup(KALIT, True):
    print("GNOME topilmadi: rejimlar tizim sozlamalarida qo'shiladi.")
    sys.exit(0)

sozlamalar = Gio.Settings.new(KALIT)
royxat = list(sozlamalar.get_value('sources').unpack()) or [('xkb', 'us')]
for arg in sys.argv[1:]:
    juft = tuple(arg[1:].split(':', 1))
    if arg[0] == '+' and juft not in royxat:
        royxat.append(juft)
    elif arg[0] == '-' and juft in royxat:
        royxat.remove(juft)
royxat = royxat or [('xkb', 'us')]
sozlamalar.set_value('sources', GLib.Variant('a(ss)', royxat))
Gio.Settings.sync()
print("✓ til ro'yxati:", ', '.join(nom for _, nom in royxat))
