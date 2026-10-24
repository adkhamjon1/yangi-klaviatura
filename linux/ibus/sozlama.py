"""Sozlamalar va foydalanuvchi lug'ati: ~/.config/yangi-klaviatura/ — dvigatel va oyna uchun umumiy."""
import json
import os
from pathlib import Path

PAPKA = Path(os.environ.get('XDG_CONFIG_HOME') or Path.home() / '.config') / 'yangi-klaviatura'
FAYL = PAPKA / 'sozlama.json'
LUGAT = PAPKA / 'lugat.txt'        # foydalanuvchining xorijiy so'zlari

STANDART = {
    'tugmalar': True,   # W → ş, C → ç, [ → ö, ] → ğ
    'lotin': True,      # sh → ş, ch → ç, o' → ö, g' → ğ
    'kirill_rejim': True,  # «ўз» rejimi til ro'yxatida
    'kirill': True,     # «ўз» da kirillcha → yangi lotin (o'chiq bo'lsa oddiy kirillcha)
    'eski': True,       # «o'z» rejimi til ro'yxatida (eski lotin yozuvi)
    'sh_ajrat': True,   # s'h → sh
    'tutuq': False,     # so'z ichidagi ' → ʼ
    'eski_belgi': False,  # «o'z» da o' → oʻ, g' → gʻ (U+02BB)
    'terminal': True,   # terminalda o'girmaslik
    'darhol': True,     # harf tagiga chizilmaydi, o'girish joyida bo'ladi (dastur qo'llasa)
    'istisno': True,    # havola, email, harf-raqamli va xorijiy harfli so'z o'girilmaydi
    'lugat': True,      # xorijiy so'zlar lug'ati (o'rnatilgan + foydalanuvchiniki)
    'organish': True,   # Backspace bilan qaytarilgan so'z lug'atga qo'shiladi
}


def oqi():
    s = dict(STANDART)
    try:
        fayl = json.loads(FAYL.read_text(encoding='utf-8'))
        s.update({k: bool(v) for k, v in fayl.items() if k in STANDART})
    except (OSError, ValueError, AttributeError):
        pass
    return s


def yoz(s):
    PAPKA.mkdir(parents=True, exist_ok=True)
    vaqtincha = FAYL.with_suffix('.tmp')
    vaqtincha.write_text(json.dumps(s, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    vaqtincha.replace(FAYL)


def lugat_sozlari():
    try:
        return [q.strip() for q in LUGAT.read_text(encoding='utf-8').splitlines()
                if q.strip() and not q.lstrip().startswith('#')]
    except OSError:
        return []


def lugat_yoz(sozlar):
    PAPKA.mkdir(parents=True, exist_ok=True)
    tartibli = sorted({s.strip() for s in sozlar if s.strip()}, key=str.lower)
    vaqtincha = LUGAT.with_suffix('.tmp')
    vaqtincha.write_text(''.join(f'{s}\n' for s in tartibli), encoding='utf-8')
    vaqtincha.replace(LUGAT)


def lugat_qosh(soz):
    sozlar = lugat_sozlari()
    if soz.strip() and soz.strip().lower() not in {s.lower() for s in sozlar}:
        lugat_yoz(sozlar + [soz])


def lugat_ochir(soz):
    lugat_yoz([s for s in lugat_sozlari() if s.lower() != soz.strip().lower()])


def kuzat(funksiya, fayl=FAYL):
    """Fayl o'zgarsa funksiya() chaqiriladi. Qaytgan kuzatuvchi saqlab turilishi kerak."""
    from gi.repository import Gio
    PAPKA.mkdir(parents=True, exist_ok=True)
    kuzatuvchi = Gio.File.new_for_path(str(fayl)).monitor_file(Gio.FileMonitorFlags.WATCH_MOVES, None)
    kuzatuvchi.connect('changed', lambda *_: funksiya())
    return kuzatuvchi
