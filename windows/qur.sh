#!/usr/bin/env bash
# Windows qismini Linux'da MinGW bilan yig'adi:  bash windows/qur.sh
#   windows/build/ozbek64.dll, ozbek32.dll — TSF matn xizmati (x64 va 32-bitli ilovalar uchun)
# Avval sinovlar (C++ yadro va dvigatel) Linux'da yuradi.
set -euo pipefail
cd "$(dirname "$0")"

VERSIYA=$(grep -oP "^VERSIYA = '\K[^']+" ../linux/ibus/sozlamalar.py)
IFS=. read -r V1 V2 V3 <<< "$VERSIYA"
mkdir -p build

python3 jadval.py --check
[ -f tsf/ikonka/oz.ico ] || python3 ikonka.py

# 1. Sinovlar (Linux, g++)
g++ -std=c++17 -O2 -Wall -Wextra -o build/sinov yadro/ogirgich.cpp yadro/sinov.cpp
g++ -std=c++17 -O2 -Wall -Wextra -o build/sinov_dvigatel yadro/ogirgich.cpp yadro/dvigatel.cpp yadro/sinov_dvigatel.cpp
build/sinov ../sinov/holatlar.tsv ../linux/ibus/lugat.txt
build/sinov_dvigatel ../linux/ibus/lugat.txt

# 2. TSF xizmati (MinGW): tizim DLL'laridan boshqa hech narsaga bog'lanmaydi
MANBA=(tsf/asosiy.cpp tsf/xizmat.cpp tsf/sozlama.cpp yadro/ogirgich.cpp yadro/dvigatel.cpp)
for arx in x86_64:64 i686:32; do
  uchlik=${arx%%:*}-w64-mingw32 bit=${arx##*:}
  $uchlik-windres -DVERSIYA_RAQAM="$V1,$V2,$V3,0" -DVERSIYA_MATN="\\\"$VERSIYA\\\"" \
    -I tsf tsf/resurs.rc -O coff -o build/resurs$bit.o
  $uchlik-g++ -std=c++17 -O2 -Wall -shared -static -static-libgcc -static-libstdc++ \
    -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX \
    -o build/ozbek$bit.dll "${MANBA[@]}" build/resurs$bit.o tsf/ozbek.def \
    -Wl,--enable-stdcall-fixup -lole32 -loleaut32 -luuid -ladvapi32 -lshell32 -luser32
  $uchlik-strip build/ozbek$bit.dll
  echo "✓ build/ozbek$bit.dll ($(du -k build/ozbek$bit.dll | cut -f1) KB): $($uchlik-objdump -p build/ozbek$bit.dll \
    | grep -oE 'DLL Name: \S+' | cut -d' ' -f3 | sort -u | tr '\n' ' ')"
done
