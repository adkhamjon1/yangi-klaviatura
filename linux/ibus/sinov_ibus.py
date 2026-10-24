#!/usr/bin/env python3
"""Jonli sinov: o'rnatilgan dvigatelga IBus orqali tugma yuboradi va chiqqan matnni tekshiradi.

    python3 linux/ibus/sinov_ibus.py
"""
import sys

import gi

gi.require_version('IBus', '1.0')
from gi.repository import GLib, IBus  # noqa: E402

M = IBus.ModifierType
SINOVLAR = [
    # dvigatel, yuboriladigan belgilar, kutilgan natija, maqsad
    ('ozbek-lotin', 'shahar', 'şahar', None),
    ('ozbek-lotin', "O'zbekiston", 'Özbekiston', None),
    ('ozbek-lotin', "g'isht choy", 'ğişt çoy', None),
    ('ozbek-lotin', "Is'hoq", 'Ishoq', None),
    ('ozbek-lotin', 'wahar', 'şahar', None),           # sxema qo'llanmagan holat: W → ş
    ('ozbek-lotin', 'ch', 'ç', None),                  # C tugmasi (ç) + eski odat h
    ('ozbek-lotin', 'shahar', 'shahar', IBus.InputPurpose.PASSWORD),
    ('ozbek-lotin', 'ssh', 'ssh', IBus.InputPurpose.TERMINAL),
    ('ozbek-kirill', 'Ўзбекистон шаҳар', 'Özbekiston şahar', None),
    ('ozbek-kirill', 'ЁЗ поезд цирк', 'YOZ poyezd sirk', None),
]


def aylan(soni=20):
    kontekst = GLib.MainContext.default()
    for _ in range(soni):
        while kontekst.pending():
            kontekst.iteration(False)
        GLib.usleep(5000)


def main():
    IBus.init()
    bus = IBus.Bus()
    if not bus.is_connected():
        sys.exit('IBus ishlamayapti')
    ic = bus.create_input_context('yangi-klaviatura-sinov')
    ic.set_capabilities(IBus.Capabilite.PREEDIT_TEXT | IBus.Capabilite.FOCUS)
    chiqdi = []
    ic.connect('commit-text', lambda _ic, matn: chiqdi.append(matn.get_text()))
    ic.focus_in()

    xato = 0
    for dvigatel, yubor, kutilgan, maqsad in SINOVLAR:
        ic.set_engine(dvigatel)
        aylan()
        ic.set_content_type(maqsad if maqsad is not None else IBus.InputPurpose.FREE_FORM, 0)
        aylan()
        chiqdi.clear()
        for belgi in yubor:
            kv = IBus.unicode_to_keyval(belgi)
            if not ic.process_key_event(kv, 0, 0):
                aylan(2)
                chiqdi.append(belgi)        # dvigatel o'tkazib yubordi — ilova o'zi yozadi
            ic.process_key_event(kv, 0, M.RELEASE_MASK)
            aylan(2)
        ic.reset()                           # oxirgi kutayotgan harf ham chiqadi
        aylan()
        natija = ''.join(chiqdi)
        belgi = '✓' if natija == kutilgan else '✗'
        xato += natija != kutilgan
        print(f'{belgi} {dvigatel:13} {yubor!r:22} → {natija!r}' + ('' if natija == kutilgan else f'  (kutilgan {kutilgan!r})'))
    ic.focus_out()
    sys.exit(1 if xato else 0)


if __name__ == '__main__':
    main()
