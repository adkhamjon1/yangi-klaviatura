#!/usr/bin/env python3
"""Jonli sinov: o'rnatilgan dvigatelga IBus orqali tugma yuboradi va chiqqan matnni tekshiradi.

    python3 linux/ibus/sinov_ibus.py           # o'rnatilgan dvigatel
    python3 linux/ibus/sinov_ibus.py --manba   # repo'dagi dvigatel (o'rnatilganiga tegmaydi)

Sinov alohida, xususiy ibus-daemon nusxasida yuradi: IBus 1.5 da dvigatel butun tizim uchun
bitta (global), uni almashtirish ish stolidagi yozishga ta'sir qilmasligi kerak. Nusxa o'z
manzil faylini vaqtinchalik papkaga yozadi (XDG_CONFIG_HOME) — aks holda u ~/.config/ibus/bus
dagi umumiy faylni egallab oladi va GNOME asosiy IBus'ni yo'qotadi (paneldan til belgisi ketadi).
Dvigatel ham shu papkadan o'qiydi, ya'ni sinov standart sozlamalar bilan yuradi.
"""
import glob
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
    # «tagiga»da butun so'z tagiga chizilgan: so'z oxirida istisno va lug'at qo'llanadi
    ('ozbek-lotin', 'www.shifo.uz info@choy.uz ', 'www.shifo.uz info@choy.uz ', None),
    ('ozbek-lotin', 'Çrome kas shahar', 'Chrome kas şahar', None),
    ('ozbek-lotin', '[]il [wa wc[] ', 'öğil öşa şçöğ ', None),
]


def aylan(soni=20):
    kontekst = GLib.MainContext.default()
    for _ in range(soni):
        while kontekst.pending():
            kontekst.iteration(False)
        GLib.usleep(5000)


def main():
    umumiy = {f: open(f, 'rb').read() for f in glob.glob(os.path.expanduser('~/.config/ibus/bus/*'))}
    papka = tempfile.mkdtemp(prefix='yangi-klaviatura-ibus-')
    manzil = f'unix:path={papka}/ibus'
    os.environ['IBUS_ADDRESS'] = manzil
    muhit = {**os.environ, 'XDG_CONFIG_HOME': f'{papka}/config'}
    if '--manba' in sys.argv:                # komponent repo'dagi engine.py ni ko'rsatadi
        bu = os.path.dirname(os.path.abspath(__file__))
        os.makedirs(f'{papka}/component')
        xml = open(f'{bu}/ozbek.xml', encoding='utf-8').read().replace('@TIZIM@', bu).replace('@VERSIYA@', 'manba')
        open(f'{papka}/component/ozbek.xml', 'w', encoding='utf-8').write(xml)
        muhit['IBUS_COMPONENT_PATH'] = f'{papka}/component'
    daemon = subprocess.Popen(
        ['ibus-daemon', '--single', '--panel=disable', '--config=disable', '--cache=none', f'--address={manzil}'],
        env=muhit, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    xato = 0
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
    for f, eski in umumiy.items():
        if not os.path.exists(f) or open(f, 'rb').read() != eski:
            print(f'✗ umumiy IBus manzil fayli o\'zgardi: {f}')
            xato += 1
    sys.exit(1 if xato else 0)


def sinovlar(bus):
    ic = bus.create_input_context('yangi-klaviatura-sinov')
    ic.set_capabilities(IBus.Capabilite.PREEDIT_TEXT | IBus.Capabilite.FOCUS | IBus.Capabilite.PROPERTY)
    chiqdi = []
    ic.connect('commit-text', lambda _ic, matn: chiqdi.append(matn.get_text()))
    xossalar = {}

    def royxatga_ol(_ic, royxat):
        xossalar.clear()
        i = 0
        while (x := royxat.get(i)) is not None:
            xossalar[x.get_key()] = x.get_state()
            i += 1
    ic.connect('register-properties', royxatga_ol)
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
    # Panel menyusidagi toggle: «Eski lotindan o'girish» o'chirilsa, sh o'zgarmay qoladi
    bus.set_global_engine('ozbek-lotin')
    aylan(40)
    togri = {'tugmalar', 'lotin', 'sozlamalar'} <= set(xossalar)
    xato += not togri
    print(f"{'✓' if togri else '✗'} panel menyusi «öz»: {', '.join(xossalar) or 'bo`sh'}")
    ic.property_activate('lotin', IBus.PropState.UNCHECKED)
    aylan(40)
    chiqdi.clear()
    for belgi in 'shahar':
        if not ic.process_key_event(IBus.unicode_to_keyval(belgi), 0, 0):
            chiqdi.append(belgi)
        aylan(2)
    ic.reset()
    aylan()
    natija = ''.join(chiqdi)
    togri = natija == 'shahar'
    xato += not togri
    print(f"{'✓' if togri else '✗'} panel toggle «lotin» o'chiq   'shahar'               → {natija!r}")
    ic.property_activate('lotin', IBus.PropState.CHECKED)
    aylan(40)
    xato += darhol_sinovlari(bus, ic, chiqdi)
    ic.focus_out()
    return xato


# «darhol» rejimi: ilova atrofdagi matnni beradi; '\b' — Backspace
DARHOL = [
    ('ozbek-lotin', 'shahar ', 'şahar '),
    ('ozbek-lotin', "O'zbekiston Is'hoq ", 'Özbekiston Ishoq '),
    ('ozbek-lotin', 'Chrome va Shell ', 'Chrome va Shell '),
    ('ozbek-lotin', 'Çrome ', 'Chrome '),
    ('ozbek-lotin', 'www.shifo.uz/chorak ', 'www.shifo.uz/chorak '),
    ('ozbek-lotin', 'info@choy.uz ', 'info@choy.uz '),
    ('ozbek-lotin', 's\b', ''),
    ('ozbek-lotin', 'shahar \b', 'shahar'),            # so'zdan keyin Backspace — eski holi
    ('ozbek-lotin', 'shahar ', 'shahar '),              # ... va lug'atga yozildi
    ('ozbek-kirill', 'шаҳар ', 'şahar '),
    ('ozbek-kirill', 'Ёз ', 'Yoz '),
]


def darhol_sinovlari(bus, ic, chiqdi):
    bufer = {'matn': '', 'kursor': 0}

    def yangila():
        ic.set_surrounding_text(IBus.Text.new_from_string(bufer['matn']), bufer['kursor'], bufer['kursor'])

    def qosh(matn):
        m, k = bufer['matn'], bufer['kursor']
        bufer['matn'], bufer['kursor'] = m[:k] + matn + m[k:], k + len(matn)
        yangila()

    def ochir(siljish, soni):
        m, k = bufer['matn'], bufer['kursor']
        bosh = max(0, k + siljish)
        bufer['matn'], bufer['kursor'] = m[:bosh] + m[bosh + soni:], bosh
        yangila()

    ulanish = [ic.connect('commit-text', lambda _ic, t: qosh(t.get_text())),
               ic.connect('delete-surrounding-text', lambda _ic, s, n: ochir(s, n))]
    ic.set_capabilities(IBus.Capabilite.PREEDIT_TEXT | IBus.Capabilite.FOCUS
                        | IBus.Capabilite.SURROUNDING_TEXT | IBus.Capabilite.PROPERTY)
    xato = 0
    for dvigatel, yubor, kutilgan in DARHOL:
        bus.set_global_engine(dvigatel)
        for _ in range(100):
            aylan(2)
            joriy = ic.get_engine()
            if joriy and joriy.get_name() == dvigatel:
                break
        ic.set_content_type(IBus.InputPurpose.FREE_FORM, 0)
        ic.reset()
        aylan()
        bufer['matn'], bufer['kursor'] = '', 0
        yangila()
        aylan()
        for belgi in yubor:
            kv = IBus.KEY_BackSpace if belgi == '\b' else IBus.unicode_to_keyval(belgi)
            if not ic.process_key_event(kv, 0, 0):
                aylan(2)
                if belgi == '\b':
                    ochir(-1, 1)
                else:
                    qosh(belgi)                 # dvigatel o'tkazib yubordi — ilova o'zi yozadi
            ic.process_key_event(kv, 0, M.RELEASE_MASK)
            aylan(2)
        aylan()
        natija = bufer['matn']
        togri = natija == kutilgan
        xato += not togri
        print(f"{'✓' if togri else '✗'} darhol {dvigatel:13} {yubor!r:24} → {natija!r}"
              + ('' if togri else f'  (kutilgan {kutilgan!r})'))
    for u in ulanish:
        ic.disconnect(u)
    return xato


if __name__ == '__main__':
    main()
