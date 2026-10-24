#!/usr/bin/env python3
"""«Özbek (yangi yozuv)» IBus dvigateli.

Uch rejim bitta kod bilan ishlaydi, farqi sxemada:
  ozbek-lotin  «öz»  — ozbek sxemasi (W → ş, C → ç, [ → ö, ] → ğ), eski lotincha
                       sh, ch, o', g' ham yangi harfga o'giriladi;
  ozbek-eski   «o'z» — US sxemasi, eski lotin yozuvi o'zgarishsiz (xohlansa o' → oʻ);
  ozbek-kirill «ўз»  — o'zbek kirill sxemasi, kirillcha yozilgan matn yangi lotinda chiqadi.

Ikki yozish usuli bor:
  darhol   — dastur atrofdagi matnni bersa (GNOME'da SURROUNDING_TEXT): harf darhol yoziladi,
             o'girish joyida bo'ladi (s → ş); so'z tugaganda lug'at tuzatadi, undan keyingi
             Backspace so'zni eski holiga qaytaradi;
  tagiga   — qolgan joyda: joriy so'z tagiga chizilib turadi va so'z oxirida (istisno va lug'at
             bilan) yoziladi — havola, email va xorijiy nom buzilmaydi.
Sozlamalar sozlama.json da turadi va o'zgarsa darhol qo'llanadi.
"""
import logging
import os
import subprocess
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
from ogirgich import LUGAT_FAYL, TINISH, Lugat, Ogirgich, eski_yozuvda, lugat_oqi  # noqa: E402
from xarita import TUGMA_ASLI  # noqa: E402

M = IBus.ModifierType
BOSHQARUV = M.CONTROL_MASK | M.MOD1_MASK | M.MOD4_MASK | M.SUPER_MASK | M.HYPER_MASK | M.META_MASK
MAXFIY = {IBus.InputPurpose.PASSWORD, IBus.InputPurpose.PIN, IBus.InputPurpose.EMAIL}
TERMINAL = getattr(IBus.InputPurpose, 'TERMINAL', None)
KURSOR = {
    IBus.KEY_Left, IBus.KEY_Right, IBus.KEY_Up, IBus.KEY_Down, IBus.KEY_Home, IBus.KEY_End,
    IBus.KEY_Page_Up, IBus.KEY_Page_Down, IBus.KEY_Return, IBus.KEY_KP_Enter, IBus.KEY_Tab,
    IBus.KEY_Escape, IBus.KEY_Delete,
}
SOZ_OXIRI = {IBus.KEY_Return, IBus.KEY_KP_Enter, IBus.KEY_Tab}
# Sxema hali qo'llanmagan bo'lsa (GNOME yangi sxemani qayta kirgach taniydi),
# W, C, [, ] oddiy belgi beradi — ularni dvigatelning o'zi yangi harfga aylantiradi.
ASLI_TUGMA = {asl: yangi for yangi, asl in TUGMA_ASLI.items()}
APOSTROF = frozenset("'’`‘")
SOZLAMALAR_OYNASI = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sozlamalar.py')
# Paneldagi til menyusida har rejimning o'z toggle'lari (keyin «Sozlamalar…» bandi)
PANEL = {
    'ozbek-lotin': [('tugmalar', 'Yangi harflar tugmalarda'), ('lotin', "Eski lotindan o'girish")],
    'ozbek-eski': [('eski_belgi', 'oʻ va gʻ belgisi')],
    'ozbek-kirill': [('kirill', "Yangi lotinga o'girish")],
}


def xossa(kalit, tur, nom, holat):
    bosh = IBus.Text.new_from_string('')
    return IBus.Property.new(kalit, tur, IBus.Text.new_from_string(nom), '', bosh, True, True, holat,
                             IBus.PropList())


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
        self.ekranda = ''           # «darhol»: joriy so'z ekranda qanday turibdi
        self.qaytarish = None       # «darhol»: (so'z + bo'shliq ekranda, eski holi)
        self.eski_oldingi = ''
        self._sozla()
        self._kuzatuvchilar = [sozlama.kuzat(self._sozla), sozlama.kuzat(self._sozla, sozlama.LUGAT)]

    def _sozla(self):
        self.s = sozlama.oqi()
        og = self.og
        og.lotin, og.kirill = self.s['lotin'], self.s['kirill']
        og.sh_ajrat, og.tutuq = self.s['sh_ajrat'], self.s['tutuq']
        og.istisnolar = self.s['istisno']
        og.lugat = Lugat(lugat_oqi(LUGAT_FAYL, sozlama.LUGAT)) if self.s['lugat'] else None
        if self.get_name():
            self.register_properties(self._xossalar())

    def _xossalar(self):
        royxat = IBus.PropList()
        for kalit, nom in PANEL.get(self.get_name(), ()):
            holat = IBus.PropState.CHECKED if self.s[kalit] else IBus.PropState.UNCHECKED
            royxat.append(xossa(kalit, IBus.PropType.TOGGLE, nom, holat))
        royxat.append(xossa('sozlamalar', IBus.PropType.NORMAL, 'Sozlamalar…  (Super+K)', IBus.PropState.UNCHECKED))
        return royxat

    def do_property_activate(self, nom, holat):
        if nom == 'sozlamalar':
            subprocess.Popen([sys.executable, SOZLAMALAR_OYNASI], start_new_session=True,
                             stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        elif nom in sozlama.STANDART:
            s = sozlama.oqi()
            s[nom] = holat == IBus.PropState.CHECKED
            sozlama.yoz(s)                      # kuzatuvchi _sozla() ni chaqiradi

    def _darhol(self):
        """Harf darhol yoziladimi: dastur atrofdagi matnni bergan bo'lsa va terminal bo'lmasa."""
        return (self.s['darhol'] and not self.terminal
                and bool(self.imkoniyat & IBus.Capabilite.SURROUNDING_TEXT))

    # -- tugmalar --

    def do_process_key_event(self, keyval, keycode, state):
        if state & M.RELEASE_MASK:
            return False
        if self.maxfiy or (self.terminal and self.s['terminal']):
            return False
        if self.get_name() == 'ozbek-eski':
            return self._eski(keyval, state)
        c_tugmadan, self.c_tugmadan = self.c_tugmadan, False
        qaytarish, self.qaytarish = self.qaytarish, None
        darhol = self._darhol()

        if state & BOSHQARUV:
            self._tasdiqla(darhol)
            self._unut()
            return False
        if keyval == IBus.KEY_BackSpace:
            return self._orqaga(darhol, qaytarish)

        asl = IBus.keyval_to_unicode(keyval).strip('\x00')
        if asl == ' ' or keyval in SOZ_OXIRI:
            return self._soz_oxiri(darhol, asl if asl == ' ' else '')
        if not asl or not asl.isprintable():
            self._tasdiqla(darhol)
            if keyval in KURSOR:
                self._unut()
            return False

        altgr = bool(state & M.MOD5_MASK)
        belgi, tugma = asl, None
        if not altgr:
            if not self.s['tugmalar']:
                belgi = TUGMA_ASLI.get(asl, asl)
            elif asl in TUGMA_ASLI:
                tugma = TUGMA_ASLI[asl]         # ozbek sxemasi: W → ş, asli w
            elif self.get_name() == 'ozbek-lotin' and asl in ASLI_TUGMA:
                belgi, tugma = ASLI_TUGMA[asl], asl
        if c_tugmadan and belgi in ('h', 'H') and self.s['lotin'] and not self.og.otkaz:
            return True                         # eski odat: C (ç) + h → ç
        if self.og.oldingi is None:
            self.og.oldingi = self._oldingi_belgi()
        self.c_tugmadan = not altgr and belgi in ('ç', 'Ç') and self.s['tugmalar']

        if darhol:
            if altgr:
                self.og.xom(belgi)
            else:
                self.og.qabul(belgi, tugma)
            oldin, yangi = self.ekranda, self.og.ekran()
            self.ekranda = yangi
            if yangi == oldin + asl:
                return False                    # hech narsa o'zgarmadi — ilovaning o'zi yozadi
            self._almashtir(oldin, yangi)
            return True

        if altgr:
            self.og.xom(belgi)
        else:
            self.og.qabul(belgi, tugma)
        self._korsat(self.og.ekran())
        return True

    def _soz_oxiri(self, darhol, chegara):
        """Bo'shliq, Enter yoki Tab: so'z tugadi — istisno va lug'at qo'llanadi."""
        if not darhol:
            self.og.yakunla()
            soz = self.og.token_chiq
            if chegara:                         # bo'shliq so'z bilan birga yoziladi
                self.og.qabul(chegara)
                self._yoz(soz + chegara)
                return True
            self._yoz(soz)                      # Enter, Tab ilovaning o'ziga boradi
            self._unut()
            return False
        xom, buzildi = self.og.token_xom, self.og.buzildi
        self.og.yakunla()
        oldin, yangi = self.ekranda, self.og.token_chiq
        self._almashtir(oldin, yangi)
        self.ekranda = ''
        if not chegara:
            self._unut()
            return False
        self.og.qabul(chegara)
        eski = eski_yozuvda(xom)
        if not buzildi and yangi and yangi != eski:
            self.qaytarish = (yangi + chegara, eski)
        if yangi != oldin:
            self.commit_text(IBus.Text.new_from_string(chegara))
            return True
        return False

    def _orqaga(self, darhol, qaytarish):
        if qaytarish and darhol:
            # so'zdan keyin darhol Backspace: so'z eski holiga qaytadi va lug'atga yoziladi
            ekrandagi, eski = qaytarish
            self.delete_surrounding_text(-len(ekrandagi), len(ekrandagi))
            self.commit_text(IBus.Text.new_from_string(eski))
            soz = eski.strip(TINISH)
            kirillmi = any('\u0400' <= c <= '\u04ff' for c in soz)
            if self.s['organish'] and len(soz) >= 2 and not kirillmi:
                sozlama.lugat_qosh(soz)             # lug'at faqat lotin so'zni biladi
            self._unut()
            self.og.oldingi = eski[-1:]
            return True
        if not darhol:
            if not self.og.ekran():             # tagiga chizilgan so'z yo'q — ilova o'zi o'chiradi
                self._unut()
                return False
            if self.og.kutish:
                self.og.orqaga()
            else:
                self.og.ochirildi()
            self._korsat(self.og.ekran())
            return True
        if self.og.kutish:
            self.og.orqaga()
            oldin, yangi = self.ekranda, self.og.ekran()
            self.ekranda = yangi
            self._almashtir(oldin, yangi)
            return True
        if self.og.token_chiq:
            self.og.ochirildi()                 # ilova o'zi o'chiradi
            self.ekranda = self.og.ekran()
        else:
            self._unut()
        return False

    def _eski(self, keyval, state):
        """«o'z»: eski lotin yozuvi o'zgarishsiz; xohlansa o' va g' dagi apostrof ʻ (U+02BB) bo'ladi."""
        oldingi, self.eski_oldingi = self.eski_oldingi, ''
        if not self.s['eski_belgi'] or state & (BOSHQARUV | M.MOD5_MASK):
            return False
        asl = IBus.keyval_to_unicode(keyval).strip('\x00')
        self.eski_oldingi = asl
        if asl in APOSTROF and oldingi in ('o', 'O', 'g', 'G'):
            self.commit_text(IBus.Text.new_from_string('ʻ'))
            return True
        return False

    # -- ekranga chiqarish --

    def _almashtir(self, oldin, yangi):
        """«darhol»: ekrandagi so'zni yangisiga almashtiradi — faqat farq qilgan qismini."""
        p = 0
        while p < min(len(oldin), len(yangi)) and oldin[p] == yangi[p]:
            p += 1
        ochir = len(oldin) - p
        if ochir:
            self.delete_surrounding_text(-ochir, ochir)
        if yangi[p:]:
            self.commit_text(IBus.Text.new_from_string(yangi[p:]))

    def _tasdiqla(self, darhol):
        """Kutayotgan harf boricha qoladi (Ctrl, strelka va hokazodan oldin)."""
        self.og.tugat()
        if darhol:
            oldin, yangi = self.ekranda, self.og.ekran()
            self.ekranda = yangi
            self._almashtir(oldin, yangi)
        else:
            self._yoz(self.og.ekran())          # tagiga chizilgan so'z boricha yoziladi
            self._unut()

    def _korsat(self, korinish):
        """«tagiga»: joriy so'z tagiga chizilib turadi (bo'sh bo'lsa yo'qoladi)."""
        if not korinish:
            if self.preedit_bor:
                self.update_preedit_text_with_mode(IBus.Text.new_from_string(''), 0, False,
                                                   IBus.PreeditFocusMode.CLEAR)
                self.preedit_bor = False
            return
        t = IBus.Text.new_from_string(korinish)
        t.append_attribute(IBus.AttrType.UNDERLINE, IBus.AttrUnderline.SINGLE, 0, len(korinish))
        self.update_preedit_text_with_mode(t, len(korinish), True, IBus.PreeditFocusMode.COMMIT)
        self.preedit_bor = True

    def _yoz(self, matn):
        """«tagiga»: tagiga chizilgan so'z o'rniga matn yoziladi."""
        self._korsat('')
        if matn:
            self.commit_text(IBus.Text.new_from_string(matn))

    def _oldingi_belgi(self):
        """Kursordan oldingi belgi (е → ye/e va ц → ts/s qoidasi uchun)."""
        if self.imkoniyat & IBus.Capabilite.SURROUNDING_TEXT:
            matn, kursor, _ = self.get_surrounding_text()
            t = matn.get_text() if matn else ''
            if 0 < kursor <= len(t):
                return t[kursor - 1]
        return ''

    def _unut(self):
        self.og.unut()
        self.ekranda = ''
        self.qaytarish = None

    # -- holat --

    def do_focus_in(self):
        self._unut()
        self.register_properties(self._xossalar())

    def do_enable(self):
        self.register_properties(self._xossalar())

    def do_focus_out(self):
        # «tagiga»: ilova tagiga chizilgan matnni o'zi yozib qo'yadi (PreeditFocusMode.COMMIT)
        self.preedit_bor = False
        self._unut()

    def do_reset(self):
        # «tagiga»: tagiga chizilgan so'zni IBus o'zi yozib qo'yadi (PreeditFocusMode.COMMIT)
        self.preedit_bor = False
        self._unut()

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
    for nom in ('ozbek-lotin', 'ozbek-eski', 'ozbek-kirill'):
        zavod.add_engine(nom, GObject.type_from_name('OzbekEngine'))
    if '--ibus' in sys.argv:
        bus.request_name('org.freedesktop.IBus.Ozbek', 0)
    sikl = GLib.MainLoop()
    bus.connect('disconnected', lambda *_: sikl.quit())
    sikl.run()


if __name__ == '__main__':
    main()
