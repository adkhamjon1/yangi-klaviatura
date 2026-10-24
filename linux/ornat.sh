#!/usr/bin/env bash
# «Özbek (yangi yozuv)» klaviaturasini shu foydalanuvchiga o'rnatadi (sudo kerak emas).
# Wayland sessiyasida (GNOME, KDE) sxema ~/.config/xkb dan o'qiladi.
set -euo pipefail
cd "$(dirname "$0")"

XKB="${XDG_CONFIG_HOME:-$HOME/.config}/xkb"
mkdir -p "$XKB/symbols" "$XKB/rules"
install -m 644 symbols/ozbek "$XKB/symbols/ozbek"

REG="$XKB/rules/evdev.xml"
if [ ! -e "$REG" ] || grep -q 'ozbek-klaviatura' "$REG"; then
  install -m 644 rules/evdev.xml "$REG"
elif ! grep -q '<name>ozbek</name>' "$REG"; then
  echo "! $REG da boshqa sxemalar bor: rules/evdev.xml dagi <layout> blokini unga o'zingiz qo'shing." >&2
fi
echo "✓ sxema: $XKB/symbols/ozbek"

if command -v gsettings >/dev/null && gsettings writable org.gnome.desktop.input-sources sources >/dev/null 2>&1; then
  joriy=$(gsettings get org.gnome.desktop.input-sources sources)
  if [[ $joriy != *"'ozbek'"* ]]; then
    if [[ $joriy == *"[]" ]]; then
      yangi="[('xkb', 'us'), ('xkb', 'ozbek')]"
    else
      yangi="${joriy%]}, ('xkb', 'ozbek')]"
    fi
    gsettings set org.gnome.desktop.input-sources sources "$yangi"
  fi
  echo "✓ GNOME til ro'yxati: $(gsettings get org.gnome.desktop.input-sources sources)"
  echo "  Almashtirish: Super+Probel"
else
  echo "GNOME topilmadi: tizim sozlamalarida «Özbek (yangi yozuv)» sxemasini qo'shing."
fi

if [ "${XDG_SESSION_TYPE:-}" = x11 ]; then
  echo "! X11 sessiyasi ~/.config/xkb ni o'qimaydi. Qo'lda yoqish:"
  echo "  setxkbmap -I\"$XKB\" -layout us,ozbek -print | xkbcomp -I\"$XKB\" - \"\$DISPLAY\""
fi
