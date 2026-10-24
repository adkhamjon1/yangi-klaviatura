// windows/jadval.py yasagan — qo'lda tahrirlanmaydi.
#pragma once

// «öz»: to'g'ri tugmalar (VK, oddiy, Shift, AltGr, AltGr+Shift)
struct OzTugma { unsigned vk; char32_t oddiy, shift, altgr, altgr_shift; };
static const OzTugma OZ_TUGMALAR[] = {
    {0x57, U'ş', U'Ş', U'w', U'W'},  // AD02
    {0x43, U'ç', U'Ç', U'c', U'C'},  // AB03
    {0xDB, U'ö', U'Ö', U'[', U'{'},  // AD11
    {0xDD, U'ğ', U'Ğ', U']', U'}'},  // AD12
};

// «ўз»: o'zbek kirill klaviaturasi (skankod, oddiy, Shift) — davlat standarti, XKB «uz»
struct KirillTugma { unsigned skankod; char32_t oddiy, shift; };
static const KirillTugma KIRILL_TUGMALAR[] = {
    {0x02, U'1', U'!'},  // AE01
    {0x03, U'2', U'"'},  // AE02
    {0x04, U'3', U'№'},  // AE03
    {0x05, U'4', U';'},  // AE04
    {0x06, U'5', U'%'},  // AE05
    {0x07, U'6', U':'},  // AE06
    {0x08, U'7', U'?'},  // AE07
    {0x09, U'8', U'*'},  // AE08
    {0x0A, U'9', U'('},  // AE09
    {0x0B, U'0', U')'},  // AE10
    {0x0C, U'ғ', U'Ғ'},  // AE11
    {0x0D, U'ҳ', U'Ҳ'},  // AE12
    {0x10, U'й', U'Й'},  // AD01
    {0x11, U'ц', U'Ц'},  // AD02
    {0x12, U'у', U'У'},  // AD03
    {0x13, U'к', U'К'},  // AD04
    {0x14, U'е', U'Е'},  // AD05
    {0x15, U'н', U'Н'},  // AD06
    {0x16, U'г', U'Г'},  // AD07
    {0x17, U'ш', U'Ш'},  // AD08
    {0x18, U'ў', U'Ў'},  // AD09
    {0x19, U'з', U'З'},  // AD10
    {0x1A, U'х', U'Х'},  // AD11
    {0x1B, U'ъ', U'Ъ'},  // AD12
    {0x1E, U'ф', U'Ф'},  // AC01
    {0x1F, U'қ', U'Қ'},  // AC02
    {0x20, U'в', U'В'},  // AC03
    {0x21, U'а', U'А'},  // AC04
    {0x22, U'п', U'П'},  // AC05
    {0x23, U'р', U'Р'},  // AC06
    {0x24, U'о', U'О'},  // AC07
    {0x25, U'л', U'Л'},  // AC08
    {0x26, U'д', U'Д'},  // AC09
    {0x27, U'ж', U'Ж'},  // AC10
    {0x28, U'э', U'Э'},  // AC11
    {0x29, U'ё', U'Ё'},  // TLDE
    {0x2B, U'\x005C', U'|'},  // BKSL
    {0x2C, U'я', U'Я'},  // AB01
    {0x2D, U'ч', U'Ч'},  // AB02
    {0x2E, U'с', U'С'},  // AB03
    {0x2F, U'м', U'М'},  // AB04
    {0x30, U'и', U'И'},  // AB05
    {0x31, U'т', U'Т'},  // AB06
    {0x32, U'ь', U'Ь'},  // AB07
    {0x33, U'б', U'Б'},  // AB08
    {0x34, U'ю', U'Ю'},  // AB09
    {0x35, U'.', U','},  // AB10
    {0x56, U'/', U'|'},  // LSGT
};
