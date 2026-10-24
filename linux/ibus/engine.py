#!/usr/bin/env python3
"""«Özbek (yangi yozuv)» IBus dvigateli.

Ikki rejim bitta kod bilan ishlaydi, farqi faqat sxemada:
  ozbek-lotin  «öz» — ozbek sxemasi (W → ş, C → ç, [ → ö, ] → ğ), eski lotincha
                      sh, ch, o', g' ham yangi harfga o'giriladi;
  ozbek-kirill «ўз» — o'zbek kirill sxemasi, kirillcha yozilgan matn yangi lotinda chiqadi.
Sozlamalar sozlama.json da turadi va o'zgarsa darhol qo'llanadi.
"""
import logging
import os
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Dvigatel IBus ichida ishlaydi va xatosi hech qayerda ko'rinmaydi — shuning uchun faylga yoziladi
JURNAL = Path(os.environ.get('XDG_CACHE_HOME') or Path.home() / '.cache') / 'yangi-klaviatura' / 'xato.log'
JURNAL.parent.mkdir(parents=True, exist_ok=True)
logging.basicConfig(filename=JURNAL, level=logging.WARNING, format='%(asctime)s %(message)s')
sys.excepthook = lambda tur, qiymat, iz: logging.error('xato', exc_info=(tur, qiymat, iz))

import gi  # noqa: E402

gi.require_version('IBus', '1.0')
from gi.repository import GLib, GObject, IBus  # noqa: E402

import sozlama  # noqa: E402
from ogirgich import Ogirgich  # noqa: E402
from xarita import TUGMA_ASLI  # noqa: E402

M = IBus.ModifierType
BOSHQARUV = M.CONTROL_MASK | M.MOD1_MASK | M.MOD4_MASK | M.SUPER_MASK | M.HYPER_MASK | M.META_MASK
MAXFIY = {IBus.InputPurpose.PASSWORD, IBus.InputPurpose.PIN}
TERMINAL = getattr(IBus.InputPurpose, 'TERMINAL', None)
KURSOR = {
    IBus.KEY_Left, IBus.KEY_Right, IBus.KEY_Up, IBus.KEY_Down, IBus.KEY_Home, IBus.KEY_End,
    IBus.KEY_Page_Up, IBus.KEY_Page_Down, IBus.KEY_Return, IBus.KEY_KP_Enter, IBus.KEY_Tab,
    IBus.KEY_Escape, IBus.KEY_BackSpace, IBus.KEY_Delete,
}
# Sxema hali qo'llanmagan bo'lsa (GNOME yangi sxemani qayta kirgach taniydi),
# W, C, [, ] oddiy belgi beradi — ularni dvigatelning o'zi yangi harfga aylantiradi.
ASLI_TUGMA = {asl: yangi for yangi, asl in TUGMA_ASLI.items()}


class OzbekEngine(IBus.Engine):
    __gtype_name__ = 'OzbekEngine'

    def __init__(self):
        super().__init__()
        self.og = Ogirgich()
        self.s = sozlama.STANDART
        self.maxfiy = False
        self.terminal = False
        self.imkoniyat = 0
        self.c_tugmadan = False
        self.preedit_bor = False
        self._sozla()
        self._kuzatuvchi = sozlama.kuzat(self._sozla)

    def _sozla(self):
        self.s = sozlama.oqi()
        self.og.lotin = self.s['lotin']
        self.og.kirill = self.s['kirill']
        self.og.sh_ajrat = self.s['sh_ajrat']
        self.og.tutuq = self.s['tutuq']

    # -- tugmalar --

    def do_process_key_event(self, keyval, keycode, state):
        if state & M.RELEASE_MASK:
            return False
        if self.maxfiy or (self.terminal and self.s['terminal']):
            return False
        c_tugmadan, self.c_tugmadan = self.c_tugmadan, False

        if state & BOSHQARUV:
            self._chiqar(self.og.tugat())
            self.og.unut()
            return False
        if keyval == IBus.KEY_BackSpace and self.og.kutish:
            self.og.orqaga()
            self._chiqar('')
            return True

        asl = IBus.keyval_to_unicode(keyval).strip('\x00')
        if not asl or not asl.isprintable():
            self._chiqar(self.og.tugat())
            if keyval in KURSOR:
                self.og.unut()
            return False

        altgr = bool(state & M.MOD5_MASK)
        belgi = asl
        if not altgr:
            if not self.s['tugmalar']:
                belgi = TUGMA_ASLI.get(asl, asl)
            elif self.get_name() == 'ozbek-lotin':
                belgi = ASLI_TUGMA.get(asl, asl)
        if c_tugmadan and belgi in ('h', 'H') and self.s['lotin']:
            return True                         # eski odat: C (ç) + h → ç
        if self.og.oldingi is None:
            self.og.oldingi = self._oldingi_belgi()

        kutgan = bool(self.og.kutish)
        matn = self.og.tugat() + self.og.xom(belgi) if altgr else self.og.qabul(belgi)
        self.c_tugmadan = not altgr and belgi in ('ç', 'Ç') and self.s['tugmalar']
        if not kutgan and not self.og.kutish and matn == asl:
            return False                        # hech narsa o'zgarmadi — ilovaning o'zi yozadi
        self._chiqar(matn)
        return True

    def _chiqar(self, matn):
        """Tasdiqlangan matnni yozadi va kutayotgan qismni tagiga chizib ko'rsatadi."""
        korinish = self.og.korinish()
        if self.preedit_bor and (matn or not korinish):
            self.update_preedit_text_with_mode(IBus.Text.new_from_string(''), 0, False,
                                               IBus.PreeditFocusMode.CLEAR)
            self.preedit_bor = False
        if matn:
            self.commit_text(IBus.Text.new_from_string(matn))
        if korinish:
            t = IBus.Text.new_from_string(korinish)
            t.append_attribute(IBus.AttrType.UNDERLINE, IBus.AttrUnderline.SINGLE, 0, len(korinish))
            self.update_preedit_text_with_mode(t, len(korinish), True, IBus.PreeditFocusMode.COMMIT)
            self.preedit_bor = True

    def _oldingi_belgi(self):
        """Kursordan oldingi belgi (е → ye/e va ц → ts/s qoidasi uchun)."""
        if self.imkoniyat & IBus.Capabilite.SURROUNDING_TEXT:
            matn, kursor, _ = self.get_surrounding_text()
            t = matn.get_text() if matn else ''
            if 0 < kursor <= len(t):
                return t[kursor - 1]
        return ''

    # -- holat --

    def do_focus_in(self):
        self.og.unut()

    def do_focus_out(self):
        # Ilova tagiga chizilgan matnni o'zi yozib qo'yadi (PreeditFocusMode.COMMIT)
        self.og.kutish = ''
        self.preedit_bor = False
        self.og.unut()

    def do_reset(self):
        self._chiqar(self.og.tugat())
        self.og.unut()

    def do_disable(self):
        self.do_reset()

    def do_set_capabilities(self, imkoniyat):
        self.imkoniyat = imkoniyat

    def do_set_content_type(self, maqsad, _belgilar):
        self.maxfiy = maqsad in MAXFIY
        self.terminal = TERMINAL is not None and maqsad == TERMINAL


def main():
    IBus.init()
    bus = IBus.Bus()
    if not bus.is_connected():
        sys.exit('IBus ishlamayapti')
    zavod = IBus.Factory.new(bus.get_connection())
    for nom in ('ozbek-lotin', 'ozbek-kirill'):
        zavod.add_engine(nom, GObject.type_from_name('OzbekEngine'))
    if '--ibus' in sys.argv:
        bus.request_name('org.freedesktop.IBus.Ozbek', 0)
    sikl = GLib.MainLoop()
    bus.connect('disconnected', lambda *_: sikl.quit())
    sikl.run()


if __name__ == '__main__':
    main()
