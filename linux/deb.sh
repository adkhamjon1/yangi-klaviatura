#!/usr/bin/env bash
# .deb paketini yig'adi:  bash linux/deb.sh  →  dist/yangi-klaviatura_<versiya>_all.deb
# Paket hamma foydalanuvchiga o'rnatiladi: dvigatel /usr/share, sxema /etc/xkb.
# Har foydalanuvchi «Özbek klaviaturasi» ilovasini bir marta ochib «Yoqish» ni bosadi.
set -euo pipefail
umask 022
cd "$(dirname "$0")"

VERSIYA=$(grep -oP "^VERSIYA = '\K[^']+" ibus/sozlamalar.py)
TIZIM=/usr/share/yangi-klaviatura
ILOVA=io.github.adkhamjon1.YangiKlaviatura
FAYLLAR=(ogirgich.py sozlama.py xarita.py lugat.txt foydalanuvchi.py engine.py sozlamalar.py)
shablon() { sed -e "s#@TIZIM@#$TIZIM#g" -e "s#@VERSIYA@#$VERSIYA#g" "$1"; }

ISH=$(mktemp -d)
trap 'rm -rf "$ISH"' EXIT
P="$ISH/yangi-klaviatura"
install -d "$P/DEBIAN" "$P$TIZIM" "$P/usr/share/ibus/component" "$P/usr/share/applications" \
           "$P/usr/share/doc/yangi-klaviatura"
for f in "${FAYLLAR[@]}"; do install -m 644 "ibus/$f" "$P$TIZIM/$f"; done
chmod 755 "$P$TIZIM/engine.py" "$P$TIZIM/sozlamalar.py" "$P$TIZIM/foydalanuvchi.py"
shablon ibus/ozbek.xml > "$P/usr/share/ibus/component/ozbek.xml"
shablon ibus/ilova.desktop > "$P/usr/share/applications/$ILOVA.desktop"
install -Dm 644 ibus/yangi-klaviatura.svg "$P/usr/share/icons/hicolor/scalable/apps/yangi-klaviatura.svg"
install -Dm 644 symbols/ozbek "$P/etc/xkb/symbols/ozbek"
install -Dm 644 rules/evdev.xml "$P/etc/xkb/rules/evdev.xml"
install -m 644 ../README.md "$P/usr/share/doc/yangi-klaviatura/README.md"
cat > "$P/usr/share/doc/yangi-klaviatura/copyright" <<EOT
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: yangi-klaviatura
Source: https://github.com/adkhamjon1/yangi-klaviatura

Files: *
Copyright: 2026 adkhamjon1
License: barcha huquqlar himoyalangan
EOT
find "$P" -type d -exec chmod 755 {} +
printf '/etc/xkb/symbols/ozbek\n/etc/xkb/rules/evdev.xml\n' > "$P/DEBIAN/conffiles"

cat > "$P/DEBIAN/control" <<EOT
Package: yangi-klaviatura
Version: $VERSIYA
Section: utils
Priority: optional
Architecture: all
Installed-Size: $(du -sk --exclude=DEBIAN "$P" | cut -f1)
Depends: python3 (>= 3.10), python3-gi, ibus (>= 1.5.22), gir1.2-ibus-1.0, gir1.2-gtk-4.0, gir1.2-adw-1 (>= 1.5)
Maintainer: adkhamjon1 <dr.rakhmatullayev@gmail.com>
Homepage: https://github.com/adkhamjon1/yangi-klaviatura
Description: Özbek klaviaturasi — yangi lotin alifbosi (Ö, Ğ, Ş, Ç)
 Lotincha, eski lotincha yoki kirillcha yozilgan matn yozish paytidayoq
 yangi o'zbek lotin alifbosiga o'giriladi. GNOME uchun IBus dvigateli
 («öz», «o'z», «ўз» rejimlari) va sozlamalar oynasi.
EOT

cat > "$P/DEBIAN/postinst" <<'EOT'
#!/bin/sh
set -e
if [ "$1" = configure ]; then
  gtk-update-icon-cache -qtf /usr/share/icons/hicolor 2>/dev/null || true
  update-desktop-database -q /usr/share/applications 2>/dev/null || true
  ibus write-cache --system >/dev/null 2>&1 || true
  echo "Özbek klaviaturasi o'rnatildi: «Özbek klaviaturasi» ilovasi ochilib, «Yoqish» bosiladi."
fi
EOT
cat > "$P/DEBIAN/postrm" <<'EOT'
#!/bin/sh
set -e
gtk-update-icon-cache -qtf /usr/share/icons/hicolor 2>/dev/null || true
update-desktop-database -q /usr/share/applications 2>/dev/null || true
ibus write-cache --system >/dev/null 2>&1 || true
EOT
chmod 755 "$P/DEBIAN/postinst" "$P/DEBIAN/postrm"

mkdir -p ../dist
dpkg-deb --root-owner-group -Zxz --build "$P" "../dist/yangi-klaviatura_${VERSIYA}_all.deb" >/dev/null
echo "✓ dist/yangi-klaviatura_${VERSIYA}_all.deb"
