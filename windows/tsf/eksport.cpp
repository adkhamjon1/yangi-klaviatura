// Sozlamalar oynasining «Sinab ko'rish» maydoni uchun: C++ yadroni tashqaridan chaqirish.
//   int ozbek_ogir(matn, natija, sigim, bayroqlar) — natija uzunligi; joy yetmasa −(kerakli uzunlik)
#include <cstring>

#include "umumiy.hpp"

extern "C" int __stdcall ozbek_ogir(const wchar_t* matn, wchar_t* natija, int sigim, unsigned bayroqlar) {
    try {
        if (!matn || !natija || sigim <= 0) return -1;
        ozbek::Sozlama s;
        s.lotin = bayroqlar & 1;
        s.kirill = bayroqlar & 2;
        s.sh_ajrat = bayroqlar & 4;
        s.tutuq = bayroqlar & 8;
        s.istisnolar = bayroqlar & 16;
        const bool lugat_bor = bayroqlar & 32;
        ozbek::Lugat lugat = lugat_bor ? xizmat::lugat_yasa() : ozbek::Lugat();
        std::u16string kirish(reinterpret_cast<const char16_t*>(matn));
        std::u16string chiqish =
            ozbek::utf16_ga(ozbek::ogir(ozbek::utf16_dan(kirish), s, lugat_bor ? &lugat : nullptr));
        if (static_cast<int>(chiqish.size()) + 1 > sigim) return -static_cast<int>(chiqish.size() + 1);
        std::memcpy(natija, chiqish.data(), chiqish.size() * sizeof(wchar_t));
        natija[chiqish.size()] = L'\0';
        return static_cast<int>(chiqish.size());
    } catch (...) {
        return -1;
    }
}
