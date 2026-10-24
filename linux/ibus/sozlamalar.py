#!/usr/bin/env python3
"""«Özbek klaviaturasi» sozlamalari: har funksiya alohida yoqiladi va o'chiriladi.

    python3 sozlamalar.py                 — oyna
    python3 sozlamalar.py --skrin a.png   — oynani rasmga oladi va yopiladi (sinov uchun)
"""
import os
import subprocess
import sys
import threading

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import gi  # noqa: E402

gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')
gi.require_version('Graphene', '1.0')
from gi.repository import Adw, Gdk, Gio, GLib, Graphene, Gtk  # noqa: E402

import foydalanuvchi  # noqa: E402
import sozlama  # noqa: E402
from ogirgich import LUGAT_FAYL, Lugat, lugat_oqi, ogir  # noqa: E402
from xarita import TUGMA_ASLI  # noqa: E402

ILOVA_ID = 'io.github.adkhamjon1.YangiKlaviatura'
VERSIYA = '1.0.0'
REJIM_MANBA = {'eski': foydalanuvchi.ESKI, 'kirill_rejim': foydalanuvchi.KIRILL}
IBUS_KOMPONENT = '/usr/share/ibus/component/ozbek.xml'
NAMUNA = "O'zbekiston — cho'l va tog'lar yurti. Тошкент — қадимий шаҳар. Chrome: www.shifo.uz"
ORNATILGAN_LUGAT = len(Lugat(lugat_oqi(LUGAT_FAYL)))

# Har rejimning o'z bo'limi: bir vaqtda bitta rejim ishlaydi, shuning uchun sozlamalari to'qnashmaydi
BANDLAR = [
    ('öz', 'Yangi lotin', "Lotincha yoziladi, yangi harflar o'zi chiqadi.", [
        ('tugmalar', 'input-keyboard-symbolic', 'Yangi harflar tugmalarda',
         "W → ş, C → ç, [ → ö, ] → ğ. Asl belgi o'ng Alt bilan yoziladi"),
        ('lotin', 'edit-find-replace-symbolic', "Eski yozuvdan o'girish",
         "sh → ş, ch → ç, o' → ö, g' → ğ"),
        ('sh_ajrat', 'format-text-strikethrough-symbolic', "s'h → sh",
         "Is'hoq → Ishoq: s va h alohida o'qiladi"),
    ]),
    ("o'z", 'Eski lotin', "Hozirgi alifbo o'zgarishsiz yoziladi.", [
        ('eski', 'view-list-symbolic', "Ro'yxatda ko'rinadi",
         "O'chirilsa «o'z» til ro'yxatidan olinadi"),
        ('eski_belgi', 'tools-check-spelling-symbolic', 'oʻ va gʻ belgisi',
         "o' → oʻ, g' → gʻ (U+02BB)"),
    ]),
    ('ўз', 'Kirill', "O'zbek kirill klaviaturasida yoziladi.", [
        ('kirill_rejim', 'view-list-symbolic', "Ro'yxatda ko'rinadi",
         "O'chirilsa «ўз» til ro'yxatidan olinadi"),
        ('kirill', 'preferences-desktop-locale-symbolic', "Yangi lotinga o'girish",
         "шаҳар → şahar, ўғил → öğil. O'chirilsa oddiy kirillcha yoziladi"),
    ]),
    (None, "Aqlli o'girish", "So'z tugaganda tekshiriladi: xorijiy so'z va havola asliga qaytadi.", [
        ('darhol', 'document-edit-symbolic', 'Darhol yozish',
         "Harf tagiga chizilmaydi, o'girish joyida bo'ladi. Biror dasturda harf ikki marta chiqsa, o'chiriladi"),
        ('istisno', 'web-browser-symbolic', "Havola va email'ga tegmaslik",
         "www.shifo.uz, info@choy.uz, ch1, Schwann, Washington o'zgarmaydi"),
        ('lugat', 'accessories-dictionary-symbolic', "Xorijiy so'zlar lug'ati",
         f"Chrome, Shell, CHARGE va yana {ORNATILGAN_LUGAT - 3} ta so'z o'girilmaydi"),
        ('organish', 'edit-undo-symbolic', "Qaytarilgan so'zni eslab qolish",
         "So'zdan keyin darhol Backspace bosilsa, so'z asliga qaytadi va lug'atga yoziladi"),
    ]),
    (None, 'Umumiy', "Parol va email maydonida o'girish doim o'chiq bo'ladi.", [
        ('tutuq', 'font-x-generic-symbolic', 'Tutuq belgisi ʼ',
         "ma'no → maʼno, маъно → maʼno (U+02BC)"),
        ('terminal', 'utilities-terminal-symbolic', "Terminalda o'girmaslik",
         "ssh, bash, chmod kabi buyruqlar o'zgarmaydi"),
    ]),
]
# Ota sozlama o'chiq bo'lsa ichki sozlama xira turadi
QARAM = {'eski_belgi': 'eski', 'kirill': 'kirill_rejim', 'organish': 'lugat'}

# Toggle bosilganda pastda chiqadigan izoh: (yoqilganda, o'chirilganda)
XABAR = {
    'tugmalar': ("W, C, [, ] tugmalarida yangi harflar chiqadi.", "W, C, [, ] tugmalari asl belgisini yozadi."),
    'lotin': ("«öz» da sh, ch, o', g' yangi harfga aylanadi.", "«öz» da sh, ch, o', g' o'zgarmaydi."),
    'sh_ajrat': ("s'h ajratiladi: Is'hoq → Ishoq.", "s'h o'zgarishsiz qoladi."),
    'eski': ("«o'z» qo'shildi: Super+Probel bilan tanlanadi, «öz» ga ta'sir qilmaydi.", "«o'z» til ro'yxatidan olindi."),
    'eski_belgi': ("«o'z» da o' → oʻ, g' → gʻ yoziladi.", "«o'z» da apostrof odatdagi ' bo'ladi."),
    'kirill_rejim': ("«ўз» qo'shildi: Super+Probel bilan tanlanadi.", "«ўз» til ro'yxatidan olindi."),
    'kirill': ("«ўз» da matn yangi lotinda chiqadi.", "«ўз» da oddiy kirillcha yoziladi."),
    'darhol': ("Harf darhol yoziladi, o'girish joyida bo'ladi.", "Qaror kutayotgan harf tagiga chizilib turadi."),
    'istisno': ("Havola, email va xorijiy so'z o'girilmaydi.", "Havola va email ham o'giriladi."),
    'lugat': ("Xorijiy so'zlar lug'ati ishlaydi.", "Lug'at o'chdi: «Chrome» ham «Çrome» bo'ladi."),
    'organish': ("Qaytarilgan so'z lug'atga yoziladi.", "Qaytarilgan so'z lug'atga yozilmaydi."),
    'tutuq': ("So'z ichidagi apostrof ʼ bo'ladi.", "Apostrof odatdagi ' bo'lib qoladi."),
    'terminal': ("Terminalda matn o'girilmaydi.", "Terminalda ham matn o'giriladi."),
}

# Rang nomlari (@card_bg_color …) libadwaita 1.5 da ham, yangisida ham ishlaydi
CSS = """
.hero { padding: 4px 0 22px; }
.hero-sarlavha { font-size: 28px; font-weight: 800; letter-spacing: -0.5px; }
.hero-izoh { font-size: 15px; }
.tugma {
  background-color: @card_bg_color;
  color: @card_fg_color;
  border-radius: 16px;
  min-width: 76px;
  padding: 10px 6px 9px;
  box-shadow: 0 0 0 1px alpha(currentColor, 0.09), 0 3px 0 0 alpha(black, 0.07),
              0 10px 18px -10px alpha(black, 0.35);
  transition: opacity 220ms ease-out;
}
.tugma-harf { font-size: 36px; font-weight: 800; color: @accent_color; }
.tugma-asl { font-size: 12px; font-weight: 700; font-family: monospace; opacity: 0.55; }
.tugma-ochiq { opacity: 0.35; }
.rejim-belgi {
  background-color: @accent_bg_color;
  color: @accent_fg_color;
  border-radius: 8px;
  padding: 3px 8px;
  font-weight: 800;
  font-size: 13px;
}
.natija .subtitle { font-size: 16px; }
"""


def tezkor_belgi(tugma):
    """Tugma ko'rinishi: libadwaita 1.8 da Adw.ShortcutLabel, eskisida Gtk.ShortcutLabel."""
    sinf = getattr(Adw, 'ShortcutLabel', None) or Gtk.ShortcutLabel
    return sinf(accelerator=tugma, valign=Gtk.Align.CENTER)


def tugma_qator(nom, bosilganda):
    """Butun qator tugma: libadwaita 1.6 da Adw.ButtonRow, eskisida bosiladigan ActionRow."""
    if hasattr(Adw, 'ButtonRow'):
        qator = Adw.ButtonRow(title=nom, end_icon_name='go-next-symbolic')
    else:
        qator = Adw.ActionRow(title=nom, activatable=True)
        qator.add_suffix(Gtk.Image(icon_name='go-next-symbolic'))
    qator.connect('activated', lambda *_: bosilganda())
    return qator


class Oyna(Adw.ApplicationWindow):
    def __init__(self, ilova):
        super().__init__(application=ilova, title='Özbek klaviaturasi')
        self.set_default_size(640, 900)
        self.s = sozlama.oqi()
        self.lugat = None
        self.lugat_qatorlari = []

        self.qatorlar = {}
        sahifa = Adw.PreferencesPage()
        sahifa.add(self._hero())
        sahifa.add(self._izoh())
        for belgi, sarlavha, izoh, qatorlar in BANDLAR:
            guruh = Adw.PreferencesGroup(title=sarlavha, description=izoh)
            if belgi:
                yorliq = Gtk.Label(label=belgi, valign=Gtk.Align.CENTER, width_chars=3)
                yorliq.add_css_class('rejim-belgi')
                guruh.set_header_suffix(yorliq)
            for kalit, ikonka, nom, tavsif in qatorlar:
                self.qatorlar[kalit] = self._almashtirgich(kalit, ikonka, nom, tavsif)
                guruh.add(self.qatorlar[kalit])
            if sarlavha == "Aqlli o'girish":
                guruh.add(self._lugat_muharriri())
            sahifa.add(guruh)
        sahifa.add(self._sinov())
        sahifa.add(self._tezkor())

        self.toastlar = Adw.ToastOverlay(child=sahifa)
        bosh = Adw.HeaderBar(title_widget=Adw.WindowTitle(title='Özbek klaviaturasi', subtitle='Yangi yozuv'))
        menyu = Gio.Menu()
        menyu.append('Ilova haqida', 'app.haqida')
        bosh.pack_end(Gtk.MenuButton(icon_name='open-menu-symbolic', menu_model=menyu, tooltip_text='Menyu'))
        self.banner = Adw.Banner(title='Rejimlar hali yoqilmagan', button_label='Yoqish')
        self.banner.connect('button-clicked', self._yoqish)
        qobiq = Adw.ToolbarView(content=self.toastlar)
        qobiq.add_top_bar(bosh)
        qobiq.add_top_bar(self.banner)
        self.set_content(qobiq)

        self._kuzatuvchi = sozlama.kuzat(lambda: GLib.idle_add(self._lugat_yangila), sozlama.LUGAT)
        self._lugat_yangila()
        self._banner_yangila()
        self._yangila()

    # -- bo'limlar --

    def _hero(self):
        guruh = Adw.PreferencesGroup()
        quti = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10, halign=Gtk.Align.CENTER)
        quti.add_css_class('hero')

        self.tugmalar_qatori = Gtk.Box(spacing=12, halign=Gtk.Align.CENTER, margin_bottom=10)
        for yangi in ('ö', 'ğ', 'ş', 'ç'):
            tugma = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
            tugma.add_css_class('tugma')
            harf = Gtk.Label(label=yangi)
            harf.add_css_class('tugma-harf')
            asl = Gtk.Label(label=TUGMA_ASLI[yangi].upper())
            asl.add_css_class('tugma-asl')
            tugma.append(harf)
            tugma.append(asl)
            self.tugmalar_qatori.append(tugma)
        quti.append(self.tugmalar_qatori)

        sarlavha = Gtk.Label(label="Yangi o'zbek yozuvi")
        sarlavha.add_css_class('hero-sarlavha')
        izoh = Gtk.Label(label="Ö, Ğ, Ş va Ç harflari 22.04.2027 dan rasmiy alifboga kiradi.",
                         wrap=True, justify=Gtk.Justification.CENTER)
        izoh.add_css_class('hero-izoh')
        izoh.add_css_class('dim-label')
        quti.append(sarlavha)
        quti.append(izoh)
        guruh.add(quti)
        return guruh

    def _izoh(self):
        guruh = Adw.PreferencesGroup()
        qator = Adw.ActionRow(
            title='Bir vaqtda bitta rejim ishlaydi',
            subtitle="Rejim Super+Probel bilan tanlanadi. Har bo'limdagi sozlama faqat o'sha rejimga "
                     "tegishli, shuning uchun ular bir-biriga xalal bermaydi.")
        qator.add_prefix(Gtk.Image(icon_name='dialog-information-symbolic'))
        guruh.add(qator)
        return guruh

    def _almashtirgich(self, kalit, ikonka, nom, tavsif):
        qator = Adw.SwitchRow(title=nom, subtitle=tavsif, active=self.s[kalit])
        qator.add_prefix(Gtk.Image(icon_name=ikonka))
        qator.connect('notify::active', self._ozgardi, kalit)
        return qator

    def _lugat_muharriri(self):
        self.lugat_qator = Adw.ExpanderRow(title="Mening lug'atim")
        self.lugat_qator.add_prefix(Gtk.Image(icon_name='bookmark-new-symbolic'))
        self.yangi_soz = Adw.EntryRow(title="So'z qo'shish (masalan, Photoshop)", show_apply_button=True)
        self.yangi_soz.connect('apply', self._soz_qosh)
        self.lugat_qator.add_row(self.yangi_soz)
        return self.lugat_qator

    def _sinov(self):
        guruh = Adw.PreferencesGroup(
            title="Sinab ko'rish",
            description="Eski lotin yoki kirill yozuvidagi matn shu yerda yangi alifboga o'giriladi.")
        self.kirish = Adw.EntryRow(title='Matn', text=NAMUNA)
        self.kirish.connect('changed', lambda *_: self._yangila())
        self.natija = Adw.ActionRow(title='Yangi yozuvda', subtitle_selectable=True)
        self.natija.add_css_class('property')
        self.natija.add_css_class('natija')
        nusxa = Gtk.Button(icon_name='edit-copy-symbolic', tooltip_text='Nusxa olish',
                           valign=Gtk.Align.CENTER)
        nusxa.add_css_class('flat')
        nusxa.connect('clicked', self._nusxa)
        self.natija.add_suffix(nusxa)
        guruh.add(self.kirish)
        guruh.add(self.natija)
        return guruh

    def _tezkor(self):
        guruh = Adw.PreferencesGroup(title='Tezkor tugmalar')
        for nom, tavsif, tugma in (
            ('Rejimni almashtirish', "öz → o'z → ўз; panelda tanlash ham mumkin", '<Super>space'),
            ('Shu oynani ochish', "Panel menyusida toggle'lar ham bor", '<Super>k'),
        ):
            qator = Adw.ActionRow(title=nom, subtitle=tavsif)
            qator.add_suffix(tezkor_belgi(tugma))
            guruh.add(qator)
        guruh.add(tugma_qator('Til sozlamalarini ochish',
                              lambda: subprocess.Popen(['gnome-control-center', 'keyboard'])))
        return guruh

    # -- hodisalar --

    def _ozgardi(self, qator, _param, kalit):
        self.s[kalit] = qator.get_active()
        sozlama.yoz(self.s)
        if kalit in REJIM_MANBA and os.path.exists(IBUS_KOMPONENT):
            foydalanuvchi.manba(REJIM_MANBA[kalit], self.s[kalit])
        self._xabar(XABAR[kalit][0 if self.s[kalit] else 1])
        self._yangila()

    def _xabar(self, matn, muddat=4):
        if hasattr(self.toastlar, 'dismiss_all'):
            self.toastlar.dismiss_all()
        self.toastlar.add_toast(Adw.Toast(title=matn, timeout=muddat))

    def _yangila(self):
        for kalit, ota in QARAM.items():
            self.qatorlar[kalit].set_sensitive(self.s[ota])
        if self.s['tugmalar']:
            self.tugmalar_qatori.remove_css_class('tugma-ochiq')
        else:
            self.tugmalar_qatori.add_css_class('tugma-ochiq')
        self.natija.set_subtitle(GLib.markup_escape_text(self._ogir()) or ' ')

    def _ogir(self):
        return ogir(self.kirish.get_text(), lotin=self.s['lotin'], kirill=self.s['kirill'],
                    sh_ajrat=self.s['sh_ajrat'], tutuq=self.s['tutuq'], istisnolar=self.s['istisno'],
                    lugat=self.lugat if self.s['lugat'] else None)

    def _nusxa(self, _tugma):
        self.get_clipboard().set(self._ogir())
        self._xabar('Nusxa olindi', 2)

    # -- lug'at --

    def _lugat_yangila(self):
        for qator in self.lugat_qatorlari:
            self.lugat_qator.remove(qator)
        self.lugat_qatorlari = []
        sozlar = sozlama.lugat_sozlari()
        for soz in sozlar:
            qator = Adw.ActionRow(title=GLib.markup_escape_text(soz))
            ochir = Gtk.Button(icon_name='user-trash-symbolic', tooltip_text="Lug'atdan olib tashlash",
                               valign=Gtk.Align.CENTER)
            ochir.add_css_class('flat')
            ochir.connect('clicked', lambda _t, s=soz: self._soz_ochir(s))
            qator.add_suffix(ochir)
            self.lugat_qator.add_row(qator)
            self.lugat_qatorlari.append(qator)
        self.lugat_qator.set_subtitle(f"{len(sozlar)} ta so'z o'girilmaydi" if sozlar
                                      else "Bo'sh: o'zingiz yoki Backspace orqali qo'shiladi")
        self.lugat = Lugat(lugat_oqi(LUGAT_FAYL, sozlama.LUGAT))
        if hasattr(self, 'natija'):
            self._yangila()
        return False

    def _soz_qosh(self, qator):
        soz = qator.get_text().strip()
        if len(soz) < 2:
            return
        sozlama.lugat_qosh(soz)
        qator.set_text('')
        self._xabar(f"«{soz}» lug'atga qo'shildi: endi o'girilmaydi.")

    def _soz_ochir(self, soz):
        sozlama.lugat_ochir(soz)
        self._xabar(f"«{soz}» lug'atdan olindi: endi odatdagidek o'giriladi.")

    # -- birinchi ishga tushirish --

    def _banner_yangila(self):
        self.banner.set_revealed(os.path.exists(IBUS_KOMPONENT) and not foydalanuvchi.yoqilganmi())

    def _yoqish(self, _banner):
        self.banner.set_button_label('Yoqilmoqda…')

        def ish():
            foydalanuvchi.ibus_qayta()
            foydalanuvchi.rejimlar_yoq()
            qoyildi, _izoh = foydalanuvchi.tezkor_qosh()
            GLib.idle_add(tugadi, qoyildi)

        def tugadi(qoyildi):
            self.banner.set_button_label('Yoqish')
            self._banner_yangila()
            self._xabar('Rejimlar yoqildi: Super+Probel bilan tanlanadi.'
                        + (' Sozlamalar: Super+K.' if qoyildi else ''), 6)
            return False

        threading.Thread(target=ish, daemon=True).start()

    # -- sinov uchun surat --

    def skrin(self, yol):
        vidjet = self.get_content()
        en, boy = vidjet.get_width(), vidjet.get_height()
        surat = Gtk.Snapshot.new()
        fon = Gdk.RGBA()
        fon.parse('#222226' if Adw.StyleManager.get_default().get_dark() else '#fafafb')
        surat.append_color(fon, Graphene.Rect().init(0, 0, en, boy))
        Gtk.WidgetPaintable.new(vidjet).snapshot(surat, en, boy)
        self.get_renderer().render_texture(surat.to_node(), None).save_to_png(yol)

    def aylantir(self, ulush):
        vidjet = self.get_content()
        while vidjet and not isinstance(vidjet, Gtk.ScrolledWindow):
            vidjet = vidjet.get_first_child() if not isinstance(vidjet, Adw.ToolbarView) \
                else vidjet.get_content()
        if vidjet:
            o = vidjet.get_vadjustment()
            o.set_value((o.get_upper() - o.get_page_size()) * ulush)


class Ilova(Adw.Application):
    def __init__(self, skrin=None):
        super().__init__(application_id=ILOVA_ID,
                         flags=Gio.ApplicationFlags.NON_UNIQUE if skrin else Gio.ApplicationFlags.DEFAULT_FLAGS)
        self.skrin_yoli = skrin
        haqida = Gio.SimpleAction.new('haqida', None)
        haqida.connect('activate', self._haqida)
        self.add_action(haqida)

    def do_startup(self):
        Adw.Application.do_startup(self)
        uslub = Gtk.CssProvider()
        uslub.load_from_string(CSS)
        Gtk.StyleContext.add_provider_for_display(Gdk.Display.get_default(), uslub,
                                                  Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

    def do_activate(self):
        oyna = self.get_active_window() or Oyna(self)
        if self.skrin_yoli:
            oyna.set_default_size(640, 1500)
            GLib.timeout_add(900, self._suratlar, oyna, [0, 0.5, 1])
        oyna.present()

    def _suratlar(self, oyna, ulushlar):
        """Sahifa yuqoridan pastgacha bir necha bo'lak qilib rasmga olinadi: a-0.png, a-1.png …"""
        ulush = ulushlar.pop(0)
        oyna.aylantir(ulush)

        def ol():
            oyna.skrin(self.skrin_yoli.replace('.png', f'-{2 - len(ulushlar)}.png'))
            if ulushlar:
                GLib.timeout_add(100, self._suratlar, oyna, ulushlar)
            else:
                self.quit()
            return False
        GLib.timeout_add(400, ol)
        return False

    def _haqida(self, *_):
        malumot = dict(application_name='Özbek klaviaturasi', application_icon='yangi-klaviatura',
                       version=VERSIYA, developer_name='adkhamjon1',
                       website='https://github.com/adkhamjon1/yangi-klaviatura',
                       comments="Yangi o'zbek lotin alifbosi (Ö, Ğ, Ş, Ç) uchun klaviatura va o'girgich.")
        if hasattr(Adw, 'AboutDialog'):
            Adw.AboutDialog(**malumot).present(self.get_active_window())
        else:
            Adw.AboutWindow(transient_for=self.get_active_window(), **malumot).present()


if __name__ == '__main__':
    skrin = sys.argv[sys.argv.index('--skrin') + 1] if '--skrin' in sys.argv else None
    sys.exit(Ilova(skrin).run([sys.argv[0]]))
