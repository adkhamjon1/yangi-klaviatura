#!/usr/bin/env bash
# «Özbek (yangi yozuv)» klaviaturasini olib tashlaydi: avval til ro'yxatidan, keyin fayllarni.
set -euo pipefail

XKB="${XDG_CONFIG_HOME:-$HOME/.config}/xkb"

if command -v gsettings >/dev/null && gsettings writable org.gnome.desktop.input-sources sources >/dev/null 2>&1; then
  joriy=$(gsettings get org.gnome.desktop.input-sources sources)
  yangi=$(sed -E "s/,? *\('xkb', 'ozbek'\)//; s/\[, */[/" <<<"$joriy")
  [ "$yangi" = "[]" ] && yangi="[('xkb', 'us')]"
  gsettings set org.gnome.desktop.input-sources sources "$yangi"
  echo "✓ GNOME til ro'yxati: $yangi"
fi

rm -f "$XKB/symbols/ozbek"
if grep -qs 'ozbek-klaviatura' "$XKB/rules/evdev.xml"; then rm -f "$XKB/rules/evdev.xml"; fi
rmdir "$XKB/symbols" "$XKB/rules" "$XKB" 2>/dev/null || true
echo "✓ olib tashlandi"
