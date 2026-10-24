// Dvigatel sinovi: ilova matnini taqlid qiluvchi bufer bilan (linux/ibus/sinov_ibus.py holatlari).
//   sinov_dvigatel LUGAT.txt
#include <fstream>
#include <iostream>
#include <sstream>

#include "dvigatel.hpp"

using namespace ozbek;

namespace {

struct Bufer {
    Matn matn, komp;
    bool buzuq = false;  // «darhol»: kursor oldidagi matn kutilganidan farq qildi
    Matn natija() const { return matn + komp; }
};

void bajar(Bufer& b, const Tugma& t, const Javob& j) {
    if (!j.oldingi.empty()) {
        if (b.matn.size() < j.oldingi.size() || b.matn.compare(b.matn.size() - j.oldingi.size(), Matn::npos, j.oldingi) != 0)
            b.buzuq = true;
        else
            b.matn.erase(b.matn.size() - j.oldingi.size());
    }
    b.matn += j.yoz;
    b.matn += j.tasdiq;
    if (j.kompozitsiya_bor) b.komp = j.kompozitsiya;
    if (j.yedi) return;
    // tugma ilovaga bordi — ilova o'zi bajaradi (bizning tahririmizdan keyin)
    if (t.maxsus == Maxsus::Backspace) {
        if (!b.matn.empty()) b.matn.pop_back();
    } else if (t.maxsus == Maxsus::Enter) {
        b.matn += U'\n';
    } else if (t.maxsus == Maxsus::Yoq && !t.boshqaruv && t.ilova) {
        b.matn += t.ilova;
    }
}

// '\b' — Backspace, '\n' — Enter, '^' — Ctrl tugmasi; «ўз» da ilova lotin harfini yozardi (?)
Matn yoz(Dvigatel& dv, const Matn& kirish, bool darhol, Lugat* organish = nullptr) {
    Bufer b;
    for (char32_t c : kirish) {
        Tugma t;
        if (c == U'\b') t.maxsus = Maxsus::Backspace;
        else if (c == U'\n') t.maxsus = Maxsus::Enter;
        else if (c == U'^') t.boshqaruv = true;
        else {
            t.belgi = c;
            t.ilova = (dv.rejim == Rejim::Kirill && c >= 0x400 && c <= 0x4FF) ? U'?' : c;
        }
        Javob j = dv.tugma(t, darhol);
        if (organish && !j.organ.empty()) {
            organish->qosh(j.organ);
            dv.sozla();
        }
        bajar(b, t, j);
    }
    return b.buzuq ? U"[BUZUQ] " + b.natija() : b.natija();
}

std::string fayl(const std::string& yol) {
    std::ifstream f(yol, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "sinov_dvigatel LUGAT.txt\n";
        return 2;
    }
    Lugat lugat;
    for (const auto& soz : lugat_qatorlari(fayl(argv[1]))) lugat.qosh(soz);

    int jami = 0, xato = 0;
    auto holat = [&](Rejim rejim, bool darhol, const Matn& kirish, const Matn& kutilgan,
                     DvigatelSozlama s = {}, bool yangi_lugat = false) {
        static Lugat organilgan;
        Dvigatel dv;
        dv.rejim = rejim;
        dv.s = s;
        dv.lugat = yangi_lugat ? &organilgan : &lugat;
        if (yangi_lugat) {
            organilgan = lugat;
        }
        dv.sozla();
        Matn natija = yoz(dv, kirish, darhol, yangi_lugat ? &organilgan : nullptr);
        ++jami;
        const char* r = rejim == Rejim::Oz ? "öz" : rejim == Rejim::Eski ? "o'z" : "ўз";
        if (natija != kutilgan) {
            ++xato;
            std::cout << "✗ " << (darhol ? "darhol " : "tagiga ") << r << " «" << utf8_ga(kirish) << "» → «"
                      << utf8_ga(natija) << "» (kutilgan «" << utf8_ga(kutilgan) << "»)\n";
        }
    };

    // «tagiga» (kompozitsiya) — linux sinov_ibus.py SINOVLAR
    holat(Rejim::Oz, false, U"shahar", U"şahar");
    holat(Rejim::Oz, false, U"O'zbekiston", U"Özbekiston");
    holat(Rejim::Oz, false, U"g'isht choy", U"ğişt çoy");
    holat(Rejim::Oz, false, U"Is'hoq", U"Ishoq");
    holat(Rejim::Oz, false, U"wahar", U"şahar");
    holat(Rejim::Oz, false, U"ch", U"ç");
    holat(Rejim::Eski, false, U"o'zbek shahar choy", U"o'zbek shahar choy");
    holat(Rejim::Kirill, false, U"Ўзбекистон шаҳар", U"Özbekiston şahar");
    holat(Rejim::Kirill, false, U"ЁЗ поезд цирк", U"YOZ poyezd sirk");
    {
        DvigatelSozlama s;
        s.ogir.lotin = false;
        holat(Rejim::Oz, false, U"shahar", U"shahar", s);
        s = {};
        s.ogir.kirill = false;
        holat(Rejim::Kirill, false, U"шаҳар", U"шаҳар", s);
    }
    holat(Rejim::Oz, false, U"s^", U"s");               // Ctrl kutayotgan harfni qoldiradi
    // «tagiga»da butun so'z kompozitsiyada: so'z oxirida istisno va lug'at qo'llanadi
    holat(Rejim::Oz, false, U"www.shifo.uz/chorak ", U"www.shifo.uz/chorak ");
    holat(Rejim::Oz, false, U"info@choy.uz ", U"info@choy.uz ");
    holat(Rejim::Oz, false, U"Chrome va Shell ", U"Chrome va Shell ");
    holat(Rejim::Oz, false, U"Çrome ", U"Chrome ");
    holat(Rejim::Oz, false, U"kas\nshahar ", U"kas\nşahar ");    // kutayotgan s Enter'dan oldin yoziladi
    holat(Rejim::Oz, false, U"kas shahar ", U"kas şahar ");
    holat(Rejim::Oz, false, U"shax\bhar ", U"şahar ");            // kompozitsiya ichida Backspace
    holat(Rejim::Oz, false, U"s\b", U"");
    holat(Rejim::Oz, false, U"wc[] ", U"şçöğ ");
    holat(Rejim::Oz, true, U"wc[] ", U"şçöğ ");
    holat(Rejim::Kirill, false, U"шаҳар\nЎзбекистон ", U"şahar\nÖzbekiston ");
    // «darhol» — linux DARHOL
    holat(Rejim::Oz, true, U"shahar ", U"şahar ");
    holat(Rejim::Oz, true, U"O'zbekiston Is'hoq ", U"Özbekiston Ishoq ");
    holat(Rejim::Oz, true, U"Chrome va Shell ", U"Chrome va Shell ");
    holat(Rejim::Oz, true, U"Çrome ", U"Chrome ");
    holat(Rejim::Oz, true, U"www.shifo.uz/chorak ", U"www.shifo.uz/chorak ");
    holat(Rejim::Oz, true, U"info@choy.uz ", U"info@choy.uz ");
    holat(Rejim::Oz, true, U"s\b", U"");
    holat(Rejim::Oz, true, U"Chrome\n", U"Chrome\n");
    holat(Rejim::Oz, true, U"shahar \bshahar ", U"shaharshahar ", {}, true);   // qaytarildi va o'rganildi
    holat(Rejim::Oz, true, U"shahar \b shahar ", U"shahar shahar ", {}, true);
    holat(Rejim::Kirill, true, U"шаҳар ", U"şahar ");
    holat(Rejim::Kirill, true, U"Ёз ", U"Yoz ");
    holat(Rejim::Kirill, true, U"шаҳар \b", U"шаҳар");   // kirillcha so'z qaytadi, lekin o'rganilmaydi
    {
        DvigatelSozlama s;
        s.eski_belgi = true;
        holat(Rejim::Eski, true, U"o'g'il", U"oʻgʻil", s);
        holat(Rejim::Eski, false, U"ma'no", U"ma'no", s);
        s.tugmalar = false;
        holat(Rejim::Oz, true, U"wahar ", U"wahar ", s);  // tugmalar o'chiq: W o'z harfi, so'z xorijiy
    }

    std::cout << (xato ? "✗ " : "✓ ") << jami - xato << "/" << jami << " o'tdi\n";
    return xato ? 1 : 0;
}
