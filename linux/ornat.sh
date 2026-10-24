#!/usr/bin/env bash
# «Özbek (yangi yozuv)» klaviaturasini o'rnatadi.
#   bash linux/ornat.sh                — to'liq: «öz», «o'z» va «ўз» rejimlari (IBus) va sozlamalar oynasi
#   bash linux/ornat.sh --faqat-sxema  — faqat W/C/[/] sxemasi, sudo'siz; GNOME uni qayta kirgach ko'radi
set -euo pipefail
cd "$(dirname "$0")"

TIZIM=/usr/local/share/yangi-klaviatura
ILOVA=io.github.adkhamjon1.YangiKlaviatura
KOMPONENT=/usr/share/ibus/component/ozbek.xml
FAYLLAR=(ogirgich.py sozlama.py xarita.py engine.py sozlamalar.py)

if [ "${1:-}" = --tizim ]; then              # root sifatida: dvigatel, oyna, ikonka
  install -d "$TIZIM"
  for f in "${FAYLLAR[@]}"; do install -m 644 "ibus/$f" "$TIZIM/$f"; done
  chmod 755 "$TIZIM/engine.py" "$TIZIM/sozlamalar.py"
  install -Dm 644 ibus/ozbek.xml "$KOMPONENT"
  install -Dm 644 ibus/ilova.desktop "/usr/local/share/applications/$ILOVA.desktop"
  install -Dm 644 ibus/yangi-klaviatura.svg /usr/local/share/icons/hicolor/scalable/apps/yangi-klaviatura.svg
  gtk-update-icon-cache -qtf /usr/local/share/icons/hicolor 2>/dev/null || true
  exit 0
fi

# 1. Sxema (W → ş, C → ç, [ → ö, ] → ğ) — shu foydalanuvchiga
XKB="${XDG_CONFIG_HOME:-$HOME/.config}/xkb"
mkdir -p "$XKB/symbols" "$XKB/rules"
install -m 644 symbols/ozbek "$XKB/symbols/ozbek"
REG="$XKB/rules/evdev.xml"
if [ ! -e "$REG" ] || grep -q 'ozbek-klaviatura' "$REG"; then
  install -m 644 rules/evdev.xml "$REG"
elif ! grep -q '<name>ozbek</name>' "$REG"; then
  echo "! $REG da boshqa sxemalar bor: rules/evdev.xml dagi <layout> blokini unga qo'shing." >&2
fi
echo "✓ sxema: $XKB/symbols/ozbek"

if [ "${1:-}" = --faqat-sxema ]; then
  python3 manbalar.py +xkb:ozbek
  echo "GNOME yangi sxemani tizimga qayta kirgandan keyin ko'radi."
  exit 0
fi

# 2. IBus dvigateli va sozlamalar oynasi — tizimga (fayllar o'zgargan bo'lsagina sudo so'raladi)
yangilash=0
for f in "${FAYLLAR[@]}"; do cmp -s "ibus/$f" "$TIZIM/$f" || yangilash=1; done
cmp -s ibus/ozbek.xml "$KOMPONENT" || yangilash=1
if [ $yangilash = 1 ]; then
  sudo bash "$PWD/ornat.sh" --tizim
  ibus write-cache >/dev/null 2>&1 || true
  ibus restart >/dev/null 2>&1 || true
  for _ in $(seq 40); do ibus list-engine 2>/dev/null | grep -q ozbek-lotin && break; sleep 0.25; done
fi
echo "✓ IBus: $(ibus list-engine 2>/dev/null | grep -c 'ozbek-') ta rejim"

# 3. GNOME til ro'yxati: oddiy sxema o'rniga IBus rejimlari
read -r eski kirill < <(python3 -c 'import sys; sys.path.insert(0, "ibus"); import sozlama; s = sozlama.oqi(); print(int(s["eski"]), int(s["kirill"]))')
qosh=(-xkb:ozbek +ibus:ozbek-lotin)
[ "$eski" = 1 ] && qosh+=(+ibus:ozbek-eski)
[ "$kirill" = 1 ] && qosh+=(+ibus:ozbek-kirill)
python3 manbalar.py "${qosh[@]}"

# 4. Sozlamalar oynasi uchun tezkor tugma (band bo'lsa qo'yilmaydi)
python3 tezkor.py qosh '<Super>k'
echo "Almashtirish: Super+Probel. Sozlamalar: Super+K yoki paneldagi til menyusi."
