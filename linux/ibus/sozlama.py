"""Sozlamalar: ~/.config/yangi-klaviatura/sozlama.json — dvigatel va sozlamalar oynasi uchun umumiy."""
import json
import os
from pathlib import Path

PAPKA = Path(os.environ.get('XDG_CONFIG_HOME') or Path.home() / '.config') / 'yangi-klaviatura'
FAYL = PAPKA / 'sozlama.json'

STANDART = {
    'tugmalar': True,   # W → ş, C → ç, [ → ö, ] → ğ
    'lotin': True,      # sh → ş, ch → ç, o' → ö, g' → ğ
    'kirill': True,     # «ўз» rejimi: kirillcha → yangi lotin
    'sh_ajrat': True,   # s'h → sh
    'tutuq': False,     # so'z ichidagi ' → ʼ
    'terminal': True,   # terminalda o'girmaslik
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


def kuzat(funksiya):
    """Fayl o'zgarsa funksiya() chaqiriladi. Qaytgan kuzatuvchi saqlab turilishi kerak."""
    from gi.repository import Gio
    PAPKA.mkdir(parents=True, exist_ok=True)
    kuzatuvchi = Gio.File.new_for_path(str(FAYL)).monitor_file(Gio.FileMonitorFlags.WATCH_MOVES, None)
    kuzatuvchi.connect('changed', lambda *_: funksiya())
    return kuzatuvchi
