// linux/ibus/engine.py dagi do_process_key_event mantig'i — platformaga bog'liq emas.
#include "dvigatel.hpp"

namespace ozbek {
namespace {

const Matn TINISH_ = U".,;:!?\"«»„“”()[]{}…—–*";

// «öz»: to'g'ri tugmadagi asl belgi → yangi harf (sxema qo'llanmagan holat uchun ham)
char32_t yangi_harf(char32_t asl) {
    switch (asl) {
        case U'w': return U'ş';
        case U'W': return U'Ş';
        case U'c': return U'ç';
        case U'C': return U'Ç';
        case U'[': return U'ö';
        case U'{': return U'Ö';
        case U']': return U'ğ';
        case U'}': return U'Ğ';
        default: return 0;
    }
}

Matn tinishsiz(const Matn& m) {
    size_t a = 0, b = m.size();
    while (a < b && TINISH_.find(m[a]) != Matn::npos) ++a;
    while (b > a && TINISH_.find(m[b - 1]) != Matn::npos) --b;
    return m.substr(a, b - a);
}

bool kirillcha(const Matn& m) {
    for (char32_t c : m)
        if (c >= 0x400 && c <= 0x4FF) return true;
    return false;
}

}  // namespace

void Dvigatel::sozla() {
    og_.s = s.ogir;
    og_.lugat = s.lugat ? lugat : nullptr;
}

void Dvigatel::unut() {
    og_.unut();
    ekranda_.clear();
    qaytarish_bor_ = false;
}

void Dvigatel::kompozitsiya_tugadi() {
    og_.kutish.clear();
    unut();
}

void Dvigatel::farq(const Matn& oldin, const Matn& yangi, Javob& j) {
    size_t p = 0;
    while (p < oldin.size() && p < yangi.size() && oldin[p] == yangi[p]) ++p;
    j.oldingi += oldin.substr(p);
    j.yoz += yangi.substr(p);
}

void Dvigatel::tasdiqla(bool darhol, Javob& j) {
    og_.tugat();
    if (darhol) {
        Matn oldin = ekranda_, yangi = og_.ekran();
        ekranda_ = yangi;
        farq(oldin, yangi, j);
    } else {
        // kompozitsiyadagi so'z hujjatga o'tadi; keyingi harf yangi kompozitsiya boshlaydi
        j.tasdiq += og_.ekran();
        j.kompozitsiya.clear();
        j.kompozitsiya_bor = true;
        unut();
    }
}

Javob Dvigatel::tugma(const Tugma& t, bool darhol) {
    Javob j;
    if (rejim == Rejim::Eski) return eski(t);
    const bool c_tugmadan = c_tugmadan_;
    c_tugmadan_ = false;
    const bool qaytarish_bor = qaytarish_bor_;
    const Qaytarish qaytarish = qaytarish_;
    qaytarish_bor_ = false;

    if (t.boshqaruv) {
        tasdiqla(darhol, j);
        unut();
        return j;
    }
    if (t.maxsus == Maxsus::Backspace) return orqaga(darhol, qaytarish_bor, qaytarish);
    if (t.belgi == U' ' || t.maxsus == Maxsus::Enter || t.maxsus == Maxsus::Tab)
        return soz_oxiri(darhol, t.belgi == U' ' ? U' ' : 0, j);
    if (t.maxsus != Maxsus::Yoq || !t.belgi) {
        tasdiqla(darhol, j);
        if (t.maxsus == Maxsus::Kursor) unut();
        return j;
    }

    char32_t belgi = t.belgi, tugma_asli = 0;
    if (!t.altgr && rejim == Rejim::Oz && s.tugmalar)
        if (char32_t y = yangi_harf(t.belgi)) belgi = y, tugma_asli = t.belgi;
    if (c_tugmadan && (belgi == U'h' || belgi == U'H') && s.ogir.lotin && !og_.otkaz) {
        j.yedi = true;  // eski odat: C (ç) + h → ç
        return j;
    }
    c_tugmadan_ = !t.altgr && (belgi == U'ç' || belgi == U'Ç') && s.tugmalar;

    if (darhol) {
        if (t.altgr) og_.xom(belgi);
        else og_.qabul(belgi, tugma_asli);
        Matn oldin = ekranda_, yangi = og_.ekran();
        ekranda_ = yangi;
        if (yangi == oldin + Matn(1, t.ilova)) return j;  // ilovaning o'zi yozadi
        farq(oldin, yangi, j);
        j.yedi = true;
        return j;
    }
    // «tagiga»: butun so'z kompozitsiyada turadi — so'z oxirida istisno va lug'at bilan yoziladi
    if (t.altgr) og_.xom(belgi);
    else og_.qabul(belgi, tugma_asli);
    j.kompozitsiya = og_.ekran();
    j.kompozitsiya_bor = true;
    j.yedi = true;
    return j;
}

Javob Dvigatel::soz_oxiri(bool darhol, char32_t chegara, Javob j) {
    if (!darhol) {
        og_.yakunla();
        j.tasdiq += og_.token_chiq;
        j.kompozitsiya.clear();
        j.kompozitsiya_bor = true;
        if (chegara) {  // bo'shliq so'z bilan birga yoziladi — tartib buzilmaydi
            og_.qabul(chegara);
            j.tasdiq += chegara;
            j.yedi = true;
        } else {
            unut();     // Enter, Tab ilovaning o'ziga boradi
        }
        return j;
    }
    const Matn xom = og_.token_xom;
    const bool buzildi = og_.buzildi;
    og_.yakunla();
    Matn oldin = ekranda_, yangi = og_.token_chiq;
    farq(oldin, yangi, j);
    ekranda_.clear();
    if (!chegara) {
        unut();
        return j;
    }
    og_.qabul(chegara);
    Matn eski = eski_yozuvda(xom);
    if (!buzildi && !yangi.empty() && yangi != eski) {
        qaytarish_bor_ = true;
        qaytarish_ = {yangi + Matn(1, chegara), eski};
    }
    if (yangi != oldin) {
        j.yoz += chegara;
        j.yedi = true;
    }
    return j;
}

Javob Dvigatel::orqaga(bool darhol, bool qaytarish_bor, const Qaytarish& q) {
    Javob j;
    if (qaytarish_bor && darhol) {
        // so'zdan keyin darhol Backspace: so'z eski holiga qaytadi va lug'atga yoziladi
        j.oldingi = q.ekrandagi;
        j.yoz = q.eski;
        Matn soz = tinishsiz(q.eski);
        if (s.organish && soz.size() >= 2 && !kirillcha(soz)) j.organ = soz;
        unut();
        if (!q.eski.empty()) oldingi_berildi(q.eski.back());
        j.yedi = true;
        return j;
    }
    if (!darhol) {
        if (og_.ekran().empty()) {  // kompozitsiya yo'q — ilova o'zi o'chiradi
            unut();
            return j;
        }
        if (!og_.kutish.empty()) og_.orqaga();
        else og_.ochirildi();
        j.kompozitsiya = og_.ekran();
        j.kompozitsiya_bor = true;
        j.yedi = true;
        return j;
    }
    if (!og_.kutish.empty()) {
        og_.orqaga();
        Matn oldin = ekranda_, yangi = og_.ekran();
        ekranda_ = yangi;
        farq(oldin, yangi, j);
        j.yedi = true;
        return j;
    }
    if (!og_.token_chiq.empty()) {
        og_.ochirildi();  // ilova o'zi o'chiradi
        ekranda_ = og_.ekran();
    } else {
        unut();
    }
    return j;
}

Javob Dvigatel::eski(const Tugma& t) {
    Javob j;
    char32_t oldingi = eski_oldingi_;
    eski_oldingi_ = 0;
    if (!s.eski_belgi || t.boshqaruv || t.altgr || t.maxsus != Maxsus::Yoq) return j;
    eski_oldingi_ = t.belgi;
    const bool apostrof = t.belgi == U'\'' || t.belgi == U'’' || t.belgi == U'`' || t.belgi == U'‘';
    if (apostrof && (oldingi == U'o' || oldingi == U'O' || oldingi == U'g' || oldingi == U'G')) {
        j.yoz = U"ʻ";
        j.yedi = true;
    }
    return j;
}

}  // namespace ozbek
