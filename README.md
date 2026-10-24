# Özbek klaviaturasi (yangi yozuv)

O'zbek lotin alifbosining yangi harflari Ö, Ğ, Ş va Ç uchun klaviatura sxemasi.
Alifbo haqidagi qonun 21.10.2026 da imzolangan va 22.04.2027 dan kuchga kiradi.
Rasmiy klaviatura standarti hali chiqmagan, shuning uchun bu mustaqil sxema hisoblanadi.

## Harflar qayerda

Asos sifatida oddiy inglizcha (US QWERTY) klaviatura olingan, faqat to'rt tugma o'zgaradi:

| Tugma | Bosilsa | Shift bilan | O'ng Alt bilan |
|---|---|---|---|
| W | ş | Ş | w, W |
| C | ç | Ç | c, C |
| [ | ö | Ö | [, { |
| ] | ğ | Ğ | ], } |

O'zbek alifbosida W va C harflari yo'q, [ va ] esa matnda kam uchraydi, shuning uchun
yangi harflar shu tugmalarga qo'yilgan. Asl belgilar yo'qolmaydi, ular o'ng Alt (AltGr) bilan
yoziladi. Tutuq belgisi (ma'no, san'at) apostrof tugmasida qoladi. Qolgan tugmalar
inglizcha klaviaturadagidek ishlaydi.

## Linux

Ubuntu 26.04 (GNOME 50, Wayland) da sinalgan. Sxema faqat shu foydalanuvchiga o'rnatiladi,
shuning uchun `sudo` kerak emas.

```bash
git clone https://github.com/adkhamjon1/ozbek-klaviatura
cd ozbek-klaviatura
bash linux/ornat.sh
```

Yuqori panelda «ÖZ» belgisi paydo bo'ladi, sxemalar Super+Probel bilan almashtiriladi.
Olib tashlash uchun `bash linux/olib-tashla.sh` ishga tushiriladi.

Skript sxemani `~/.config/xkb` ga yozadi. Bu papkani Wayland sessiyasi (GNOME, KDE) o'qiydi.
X11 sessiyasi uni o'qimaydi, shuning uchun skript bu holda qo'lda yoqish buyrug'ini chiqaradi.

## Windows va macOS

Tayyorlanmoqda.

## Xaritani o'zgartirish

Harflarning joyi `xarita.mjs` faylida yozilgan. Uni o'zgartirgandan keyin sxema fayllari
qayta yasaladi (Node.js 18 yoki undan yangisi kerak):

```bash
node qur.mjs           # linux/ dagi fayllarni yasaydi
node qur.mjs --check   # fayllar xaritaga mosligini tekshiradi
```
