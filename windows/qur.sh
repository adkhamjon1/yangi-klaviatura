#!/usr/bin/env bash
# Windows qismini Linux'da yig'adi:  bash windows/qur.sh
#   windows/build/ozbek64.dll, ozbek32.dll — TSF matn xizmati (x64 va 32-bitli ilovalar uchun; MinGW)
#   windows/build/Sozlamalar.exe — sozlamalar oynasi (.NET Framework 4.8; Mono mcs)
#   windows/build/ozbek-klaviaturasi-<versiya>.exe — o'rnatuvchi (NSIS)
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
MANBA=(tsf/asosiy.cpp tsf/xizmat.cpp tsf/sozlama.cpp tsf/eksport.cpp yadro/ogirgich.cpp yadro/dvigatel.cpp)
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

# 3. Sozlamalar oynasi
mcs -sdk:4.8 -target:winexe -optimize+ -codepage:utf8 -nologo -win32icon:tsf/ikonka/ilova.ico \
  -r:System.Windows.Forms.dll -r:System.Drawing.dll -r:System.Core.dll -out:build/Sozlamalar.exe oyna/Sozlamalar.cs
echo "✓ build/Sozlamalar.exe ($(du -k build/Sozlamalar.exe | cut -f1) KB)"

# 4. O'rnatuvchi
python3 ikonka.py --yon build/yon.bmp > /dev/null
makensis -V2 -DVERSIYA="$VERSIYA" ornatuvchi/yangi-klaviatura.nsi
echo "✓ build/ozbek-klaviaturasi-$VERSIYA.exe ($(du -k build/ozbek-klaviaturasi-$VERSIYA.exe | cut -f1) KB)"
