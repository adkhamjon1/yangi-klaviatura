// Özbek (yangi yozuv) klaviaturasi — yagona xarita.
//
// Asos — inglizcha US QWERTY. Bu yerda faqat o'zgargan tugmalar turadi:
// o'zbek lotin alifbosida W va C harfi yo'q, [ va ] matnda kam uchraydi.
// Asl belgi yo'qolmaydi — o'ng Alt (AltGr) bilan yoziladi.
//
// Xaritani o'zgartirgach: node qur.mjs

export const NOM = 'Özbek (yangi yozuv)';
export const QISQA = 'ÖZ';
export const LINUX_ID = 'ozbek';   // XKB sxema nomi (faqat ASCII)

// Tugma nomi — XKB bo'yicha (AD02 = W, AB03 = C, AD11 = [, AD12 = ]).
export const XARITA = {
  //       oddiy  Shift  AltGr  AltGr+Shift
  AD02: ['ş', 'Ş', 'w', 'W'],
  AB03: ['ç', 'Ç', 'c', 'C'],
  AD11: ['ö', 'Ö', '[', '{'],
  AD12: ['ğ', 'Ğ', ']', '}'],
};
