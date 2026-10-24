# Özbek klaviaturasi (yangi yozuv)

O'zbek lotin alifbosining yangi harflari Ö, Ğ, Ş va Ç uchun klaviatura va o'girgich.
Alifbo haqidagi qonun 21.10.2026 da imzolangan va 22.04.2027 dan kuchga kiradi.
Rasmiy klaviatura standarti hali chiqmagan, shuning uchun bu mustaqil loyiha hisoblanadi.

Odam o'ziga qulay yozuvda yozadi, matn esa yangi alifboda chiqadi. Yangi harfni
klaviaturadan qidirish shart emas.

## Rejimlar

| Panelda | Kim uchun | Nima bo'ladi |
|---|---|---|
| **öz** | Lotincha yozuvchilar | W, C, [, ] tugmalarida yangi harflar turadi; eski yozuvdagi sh, ch, o', g' ham o'zi yangi harfga aylanadi |
| **o'z** | Eski lotinda yozish kerak bo'lganda | Hozirgi alifbo (o', g', sh, ch) o'zgarishsiz yoziladi; apostrof odatdagidek ' tugmasida |
| **ўз** | Kirillcha yozuvchilar | Odatdagi o'zbek kirill klaviaturasida yoziladi, matn yangi lotinda chiqadi; o'girish o'chirilsa oddiy kirillcha yoziladi |

Bir vaqtda bitta rejim ishlaydi, rejim Super+Probel (Windows'da Win+Probel) bilan almashtiriladi.
Har rejimning sozlamasi faqat o'sha rejimga tegishli. Windows'da rejim nomi katta harf bilan
yoziladi: ÖZ, O'Z, ЎЗ.

### Tugmalar («öz»)

| Tugma | Bosilsa | Shift bilan | O'ng Alt bilan |
|---|---|---|---|
| W | ş | Ş | w, W |
| C | ç | Ç | c, C |
| [ | ö | Ö | [, { |
| ] | ğ | Ğ | ], } |

O'zbek alifbosida W va C harflari yo'q, [ va ] esa matnda kam uchraydi, shuning uchun
yangi harflar shu tugmalarga qo'yilgan. Asl belgilar o'ng Alt (AltGr) bilan yoziladi.
Havola yoki email ichida W va C o'z harfini beradi (www., info@c…).

### O'girish

| Yozilgani | Chiqadigani |
|---|---|
| shahar, choy, o'zbek, g'isht | şahar, çoy, özbek, ğişt |
| SHAHAR, O'zbekiston, AQSH | ŞAHAR, Özbekiston, AQŞ |
| Is'hoq (s va h alohida) | Ishoq |
| ma'no, san'at (tutuq belgisi) | ma'no, san'at |
| Ўзбекистон, шаҳар, ўғил | Özbekiston, şahar, öğil |
| ер, поезд, цирк, лицей | yer, poyezd, sirk, litsey |

### Aqlli o'girish

Har so'z tugaganda (bo'shliq, Enter, Tab) tekshiriladi:

- **Havola, email va qisqartma o'zgarmaydi:** www.shifo.uz, info@choy.uz, @nom, ch1, TSH, CH-III.
- **O'zbekchada yo'q harfli so'z o'zgarmaydi:** w, h siz c, ö, ü… (Schwann, Cochrane, Washington, Pöschl).
- **Xorijiy so'zlar lug'ati:** Chrome, Shell, CHARGE, Shrapnell, cholesteatoma, tracheostomy va
  jami 184 ta so'z asliga qaytadi. «öz» da C tugmasi bilan yozilgan «Çrome» ham taniladi.
  Chiziqcha bilan yozilgan nom qismlab tekshiriladi: C-uchli → C-uçli, Arnold-Chiari o'zgarmaydi.
- **Backspace bilan qaytarish:** so'zdan keyin darhol bosilgan Backspace so'zni eski holiga qaytaradi
  va uni foydalanuvchi lug'atiga yozadi. Keyingi safar bu so'z o'girilmaydi.
- **Parol va email maydonida** o'girish doim o'chiq bo'ladi, terminalda esa sozlamaga ko'ra.

Dastur atrofdagi matnni bersa (GNOME'dagi ko'pchilik dasturlar), harf darhol yoziladi va
o'girish joyida bo'ladi. Qolgan dasturlarda qaror kutayotgan harf (masalan «s» dan keyin
«h» kelishi mumkin) tagiga chizilib turadi.

### Aniqlik

O'girgich ENT Medical saytining 277 ta mavzusida (165 824 qator) sinalgan va saytning
mustaqil o'giruvchisi bilan so'zma-so'z solishtirilgan. sh, ch, o' yoki g' bor 449 533 so'zda
natija 99,78 % mos keldi. Farq qilgan 971 holatning hammasi qo'lda ko'rib chiqildi:
346 tasi matn belgilash qoldig'i, qolganlarida bu o'girgichning javobi to'g'ri bo'lib chiqdi.

### Sozlamalar

«Özbek klaviaturasi» ilovasida har funksiya alohida yoqiladi va o'chiriladi. Ilova Super+K
bilan yoki yuqori paneldagi til menyusidan ochiladi. Panel menyusining o'zida ham o'sha
rejimning asosiy toggle'lari turadi. Ilovada shaxsiy lug'at va matnni sinab ko'rish maydoni bor.

## Linux

Ubuntu 26.04 (GNOME 50, Wayland, IBus) da sinalgan. Ubuntu 24.04 va undan yangi tizimda ishlashi
kerak (libadwaita 1.5 dan boshlab).

### Paket bilan

```bash
sudo apt install ./yangi-klaviatura_1.0.0_all.deb
```

O'rnatilgach «Özbek klaviaturasi» ilovasi ochiladi va tepadagi «Yoqish» tugmasi bosiladi:
rejimlar til ro'yxatiga qo'shiladi, Super+K qo'yiladi. Tizimdan bir marta chiqib qayta
kirilgandan keyin o'ng Alt bilan asl belgilar (w, c, [, ]) ham ishlaydi.

Olib tashlash: `sudo apt remove yangi-klaviatura`.

### Manbadan

```bash
git clone https://github.com/adkhamjon1/yangi-klaviatura
cd yangi-klaviatura
bash linux/ornat.sh                # /usr/local ga, sudo so'raydi
bash linux/ornat.sh --faqat-sxema  # faqat W/C/[/] sxemasi, sudo'siz
bash linux/olib-tashla.sh
```

Paket va manbadan o'rnatish bir vaqtda ishlatilmaydi: ikkalasi bitta IBus komponentini yozadi.
Dvigatel xatolari `~/.cache/yangi-klaviatura/xato.log` ga yoziladi.

### IBus'ga ulanmaydigan dasturlar

O'girish IBus orqali ishlaydi, W/C/[/] tugmalari esa klaviatura sxemasida turadi. Snap'dagi VS Code
kabi ba'zi Electron dasturlarida IBus moduli bo'lmaydi: ularda yangi harflar tugmalardan chiqadi,
lekin sh, ch, o', g' o'girilmaydi. Bunday dastur XIM orqali ulanadi:

```bash
env GTK_IM_MODULE=xim XMODIFIERS=@im=ibus /snap/bin/code
```

Doimiy qilish uchun dasturning `.desktop` yorlig'i `~/.local/share/applications/` ga nusxalanadi
va `Exec=` qatori boshiga `env GTK_IM_MODULE=xim XMODIFIERS=@im=ibus` qo'shiladi. Dastur to'liq
yopilib, menyu yoki dock'dan qayta ochilishi kerak: o'zini o'zi qayta ishga tushirgan dastur eski
muhitni meros oladi. XIM ko'prigi `ibus-x11` ishlab turishi shart.

## Matnni o'girish

Tayyor matn ham buyruq qatorida yangi yozuvga o'giriladi (istisnolar va lug'at bilan):

```bash
python3 linux/ibus/ogirgich.py "O'zbekiston — Ўзбекистон"
python3 linux/ibus/ogirgich.py < eski.txt > yangi.txt
```

## Windows

Windows 10 va 11 uchun. **Haqiqiy Windows'da hali sinalmagan.** O'girish yadrosi Linux'dagi
bilan bir xil ishlashi sinovlar bilan tekshirilgan (100 umumiy holat va 165 824 qatorli korpus
bayt-bayt mos). O'rnatuvchi va olib tashlovchi Wine'da sinalgan. Wine'da matn xizmatini
ro'yxatga olish ishlamaydi, shuning uchun yozishning o'zi Windows'da sinaladi.

### O'rnatish

`ozbek-klaviaturasi-1.0.0.exe` ishga tushiriladi, administrator huquqi so'raladi. O'rnatuvchi
matn xizmatini (TSF) 64 va 32-bitli dasturlar uchun ro'yxatga oladi va «ÖZ», «O'Z», «ЎЗ»
rejimlarini o'zbek (lotin) tiliga qo'shadi. Til ro'yxatda bo'lmasa, Windows uni o'zi qo'shadi.
Start menyusida «Özbek klaviaturasi» sozlamalar oynasi paydo bo'ladi.

Rejim Win+Probel bilan almashtiriladi. Windows'da W, C, [, ] tugmalari alohida klaviatura sxemasida
emas, matn xizmatining o'zida turadi. Asl belgilar o'ng Alt (AltGr) bilan yoziladi.

Sozlamalar `HKCU\Software\YangiKlaviatura` da, shaxsiy lug'at esa `%APPDATA%\YangiKlaviatura\lugat.txt`
da saqlanadi. Dastur olib tashlanganda ular o'chirilmaydi.

Olib tashlash uchun Sozlamalar → Ilovalar → «Özbek klaviaturasi» ochiladi. Jimgina olib tashlash
buyrug'i: `"C:\Program Files\YangiKlaviatura\olib-tashlash.exe" /S`.

### Yig'ish

Windows qismi Linux'da yig'iladi:

```bash
sudo apt install g++-mingw-w64-x86-64-win32 g++-mingw-w64-i686-win32 mono-devel nsis python3-pil
bash windows/qur.sh   # sinovlar → ozbek64.dll, ozbek32.dll → Sozlamalar.exe → o'rnatuvchi
```

Natija `windows/build/` papkasida chiqadi. DLL'lar faqat Windows'ning o'z kutubxonalariga bog'lanadi.

## macOS

Tayyorlanmoqda.

## Ishlab chiqish

Tugmalarning joyi `xarita.mjs` faylida yozilgan. U o'zgargandan keyin sxema fayllari
qayta yasaladi (Node.js 18 yoki undan yangisi kerak):

```bash
node qur.mjs                                      # linux/ dagi sxema va xarita fayllari
node qur.mjs --check                              # fayllar xaritaga mosligi
python3 -m unittest linux/ibus/test_ogirgich.py   # o'girish qoidalari
python3 linux/ibus/sinov_ibus.py                  # o'rnatilgan dvigatel, alohida IBus nusxasida
bash linux/deb.sh                                 # dist/yangi-klaviatura_<versiya>_all.deb
```
