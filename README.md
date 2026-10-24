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
| **ўз** | Kirillcha yozuvchilar | Odatdagi o'zbek kirill klaviaturasida yoziladi, matn yangi lotinda chiqadi |

Rejimlar Super+Probel bilan almashtiriladi.

### Tugmalar («öz»)

| Tugma | Bosilsa | Shift bilan | O'ng Alt bilan |
|---|---|---|---|
| W | ş | Ş | w, W |
| C | ç | Ç | c, C |
| [ | ö | Ö | [, { |
| ] | ğ | Ğ | ], } |

O'zbek alifbosida W va C harflari yo'q, [ va ] esa matnda kam uchraydi, shuning uchun
yangi harflar shu tugmalarga qo'yilgan. Asl belgilar o'ng Alt (AltGr) bilan yoziladi.
Qolgan tugmalar inglizcha (US) klaviaturadagidek ishlaydi.

### O'girish

| Yozilgani | Chiqadigani |
|---|---|
| shahar, choy, o'zbek, g'isht | şahar, çoy, özbek, ğişt |
| SHAHAR, O'zbekiston | ŞAHAR, Özbekiston |
| Is'hoq (s va h alohida) | Ishoq |
| ma'no, san'at (tutuq belgisi) | ma'no, san'at |
| Ўзбекистон, шаҳар, ўғил | Özbekiston, şahar, öğil |
| ер, поезд, цирк, лицей | yer, poyezd, sirk, litsey |

Hali qaror kutayotgan harf (masalan «s» dan keyin «h» kelishi mumkin) tagiga chizilgan
holda ko'rinadi va keyingi tugma bilan tasdiqlanadi.

### Sozlamalar

«Özbek klaviaturasi» ilovasida har funksiya alohida yoqiladi va o'chiriladi:
tugmalardagi harflar, eski lotin va kirill yozuvidan o'girish, `s'h → sh`, tutuq belgisi `ʼ`,
terminalda o'girmaslik, «o'z» rejimida rasmiy oʻ va gʻ belgisi (U+02BB). Ilovada matnni sinab ko'rish maydoni ham bor. Parol maydonida
o'girish doim o'chiq bo'ladi.

## Linux

Ubuntu 26.04 (GNOME 50, Wayland, IBus) da sinalgan. Kerakli paketlar: Python 3, PyGObject,
IBus, GTK 4 va libadwaita 1.6 yoki undan yangisi (Ubuntu'da odatda o'rnatilgan bo'ladi).

```bash
git clone https://github.com/adkhamjon1/yangi-klaviatura
cd yangi-klaviatura
bash linux/ornat.sh
```

Skript dvigatelni tizimga qo'yish uchun `sudo` parolini so'raydi. Rejimlar darhol panelda
paydo bo'ladi. Tizimdan bir marta chiqib qayta kirilgandan keyin o'ng Alt bilan asl
belgilar (w, c, [, ]) ham ishlaydi.

- Faqat tugmalar sxemasi, `sudo`'siz: `bash linux/ornat.sh --faqat-sxema`.
- Olib tashlash: `bash linux/olib-tashla.sh`.
- Dvigatel xatolari `~/.cache/yangi-klaviatura/xato.log` ga yoziladi.

## Matnni o'girish

Tayyor matn ham buyruq qatorida yangi yozuvga o'giriladi:

```bash
python3 linux/ibus/ogirgich.py "O'zbekiston — Ўзбекистон"
python3 linux/ibus/ogirgich.py < eski.txt > yangi.txt
```

## Windows va macOS

Tayyorlanmoqda.

## Ishlab chiqish

Tugmalarning joyi `xarita.mjs` faylida yozilgan. U o'zgargandan keyin sxema fayllari
qayta yasaladi (Node.js 18 yoki undan yangisi kerak):

```bash
node qur.mjs                                      # linux/ dagi sxema va xarita fayllari
node qur.mjs --check                              # fayllar xaritaga mosligi
python3 -m unittest linux/ibus/test_ogirgich.py   # o'girish qoidalari
python3 linux/ibus/sinov_ibus.py                  # o'rnatilgan dvigatelni IBus orqali sinash
```
