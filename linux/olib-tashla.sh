#!/usr/bin/env bash
# «Özbek (yangi yozuv)» klaviaturasini olib tashlaydi: avval til ro'yxatidan, keyin fayllarni.
# Sozlamalar fayli (~/.config/yangi-klaviatura) qoladi.
set -euo pipefail
cd "$(dirname "$0")"

python3 manbalar.py -xkb:ozbek -ibus:ozbek-lotin -ibus:ozbek-kirill

if [ -e /usr/share/ibus/component/ozbek.xml ]; then
  sudo rm -f /usr/share/ibus/component/ozbek.xml \
    /usr/local/share/applications/io.github.adkhamjon1.YangiKlaviatura.desktop \
    /usr/local/share/icons/hicolor/scalable/apps/yangi-klaviatura.svg
  sudo rm -rf /usr/local/share/yangi-klaviatura
  ibus write-cache >/dev/null 2>&1 || true
  ibus restart >/dev/null 2>&1 || true
fi

XKB="${XDG_CONFIG_HOME:-$HOME/.config}/xkb"
rm -f "$XKB/symbols/ozbek"
if grep -qs 'ozbek-klaviatura' "$XKB/rules/evdev.xml"; then rm -f "$XKB/rules/evdev.xml"; fi
rmdir "$XKB/symbols" "$XKB/rules" "$XKB" 2>/dev/null || true
echo "✓ olib tashlandi"
