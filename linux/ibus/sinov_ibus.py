#!/usr/bin/env python3
"""Jonli sinov: o'rnatilgan dvigatelga IBus orqali tugma yuboradi va chiqqan matnni tekshiradi.

    python3 linux/ibus/sinov_ibus.py

Sinov alohida, xususiy ibus-daemon nusxasida yuradi: IBus 1.5 da dvigatel butun tizim uchun
bitta (global), uni almashtirish ish stolidagi yozishga ta'sir qilmasligi kerak.
"""
import os
import shutil
import subprocess
import sys
import tempfile
import time

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
    ('ozbek-eski', "o'zbek shahar choy", "o'zbek shahar choy", None),
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
    papka = tempfile.mkdtemp(prefix='yangi-klaviatura-ibus-')
    manzil = f'unix:path={papka}/ibus'
    os.environ['IBUS_ADDRESS'] = manzil
    daemon = subprocess.Popen(
        ['ibus-daemon', '--single', '--panel=disable', '--config=disable', '--cache=none', f'--address={manzil}'],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        for _ in range(100):
            if os.path.exists(f'{papka}/ibus'):
                break
            time.sleep(0.05)
        IBus.init()
        bus = IBus.Bus()
        if not bus.is_connected():
            sys.exit('Xususiy IBus nusxasiga ulanib bo\'lmadi')
        xato = sinovlar(bus)
        bus.exit(False)
        daemon.wait(timeout=5)
    finally:
        if daemon.poll() is None:
            daemon.terminate()
        shutil.rmtree(papka, ignore_errors=True)
    sys.exit(1 if xato else 0)


def sinovlar(bus):
    ic = bus.create_input_context('yangi-klaviatura-sinov')
    ic.set_capabilities(IBus.Capabilite.PREEDIT_TEXT | IBus.Capabilite.FOCUS)
    chiqdi = []
    ic.connect('commit-text', lambda _ic, matn: chiqdi.append(matn.get_text()))
    ic.focus_in()

    xato = 0
    for dvigatel, yubor, kutilgan, maqsad in SINOVLAR:
        bus.set_global_engine(dvigatel)
        for _ in range(100):                 # dvigatel almashguncha kutiladi (birinchi marta sekinroq)
            aylan(2)
            joriy = ic.get_engine()
            if joriy and joriy.get_name() == dvigatel:
                break
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
    return xato


if __name__ == '__main__':
    main()
