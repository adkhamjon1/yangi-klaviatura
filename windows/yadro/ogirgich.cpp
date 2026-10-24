// Eski o'zbek yozuvi → yangi lotin alifbosi. linux/ibus/ogirgich.py ning aynan nusxasi:
// qoida o'zgarsa ikkalasi birga o'zgaradi va sinov/holatlar.tsv ikkalasini tekshiradi.
#include "ogirgich.hpp"

#include <unordered_set>

namespace ozbek {
namespace {

const Matn APOSTROF = U"'`‘’ʻʼ";
constexpr char32_t TUTUQ = U'ʼ';  // U+02BC
const Matn UNLI = U"aeiouöAEIOUÖ";
const Matn TINISH = U".,;:!?\"«»„“”()[]{}…—–*";
const Matn LOTIN_BOSH = U"sScCoOgG";
const Matn KIRILL_ORINGA_QARAB = U"ецъь";
const Matn QISM_AJRATGICH = U"-/–";
const Matn NOTANISH = U"äöüßéèêëáàâïñøåæ";  // eski o'zbek lotinida yo'q harf (kichik holda)
constexpr char32_t KATTA_KIRILL = 0;        // kutish belgisi: Ё → «Yo» yoki «YO»

bool ichida(const Matn& to, char32_t c) { return to.find(c) != Matn::npos; }
bool ascii_harf(char32_t c) { return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z'); }
bool raqam(char32_t c) { return c >= U'0' && c <= U'9'; }
bool kirill_harfi(char32_t c) { return c >= 0x0400 && c <= 0x04FF; }
bool kirill_bor(const Matn& m) {
    for (char32_t c : m)
        if (kirill_harfi(c)) return true;
    return false;
}

const Matn* kirill_lotin(char32_t past) {
    static const std::unordered_map<char32_t, Matn> jadval = {
        {U'а', U"a"}, {U'б', U"b"}, {U'в', U"v"}, {U'г', U"g"}, {U'д', U"d"}, {U'ё', U"yo"},
        {U'ж', U"j"}, {U'з', U"z"}, {U'и', U"i"}, {U'й', U"y"}, {U'к', U"k"}, {U'л', U"l"},
        {U'м', U"m"}, {U'н', U"n"}, {U'о', U"o"}, {U'п', U"p"}, {U'р', U"r"}, {U'с', U"s"},
        {U'т', U"t"}, {U'у', U"u"}, {U'ф', U"f"}, {U'х', U"x"}, {U'ч', U"ç"}, {U'ш', U"ş"},
        {U'щ', U"ş"}, {U'ы', U"i"}, {U'э', U"e"}, {U'ю', U"yu"}, {U'я', U"ya"}, {U'ў', U"ö"},
        {U'қ', U"q"}, {U'ғ', U"ğ"}, {U'ҳ', U"h"},
    };
    auto it = jadval.find(past);
    return it == jadval.end() ? nullptr : &it->second;
}

Matn chap_tozala(const Matn& s, const Matn& belgilar) {
    size_t i = 0;
    while (i < s.size() && ichida(belgilar, s[i])) ++i;
    return s.substr(i);
}
Matn ong_tozala(const Matn& s, const Matn& belgilar) {
    size_t n = s.size();
    while (n > 0 && ichida(belgilar, s[n - 1])) --n;
    return s.substr(0, n);
}
Matn tozala(const Matn& s, const Matn& belgilar) { return ong_tozala(chap_tozala(s, belgilar), belgilar); }

bool kattami(char32_t c) { return kichik(c) != c; }                       // harf va katta
bool kattami(const Matn& m) {                                             // Python str.isupper()
    bool harf_bor = false;
    for (char32_t c : m) {
        if (kichik(c) != c) harf_bor = true;
        else if (katta(c) != c) return false;
    }
    return harf_bor;
}
Matn bosh_harf(const Matn& m) {                                           // Python str.capitalize()
    Matn r = kichik(m);
    if (!r.empty()) r[0] = katta(r[0]);
    return r;
}

Matn harf_holati(const Matn& asl, const Matn& namuna) {
    if (namuna.size() > 1 && kattami(namuna)) return katta(asl);
    Matn r = asl;
    if (r.empty()) return r;
    r[0] = (!namuna.empty() && kattami(namuna[0])) ? katta(r[0]) : kichik(r[0]);
    return r;
}

bool xorijiy(const Matn& s) {
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == U'w' || s[i] == U'W') return true;
        if ((s[i] == U'c' || s[i] == U'C') && i + 1 < s.size() && s[i + 1] != U'h' && s[i + 1] != U'H')
            return true;
    }
    return false;
}

bool qisqartmami(const Matn& soz) {
    Matn t = tozala(soz, TINISH), qism;
    auto tekshir = [](const Matn& q) {
        if (q.size() < 2) return false;
        for (char32_t c : q)
            if (c < U'A' || c > U'Z' || ichida(U"AEIOU", c)) return false;
        return true;
    };
    for (char32_t c : t) {
        if (c == U'-' || c == U'/') {
            if (tekshir(qism)) return true;
            qism.clear();
        } else {
            qism += c;
        }
    }
    return tekshir(qism);
}

bool dis_h(const Matn& soz) {  // dis + gormon: s va h alohida (dishormonogenetik)
    Matn k = kichik(soz);
    for (size_t i = k.find(U"dish"); i != Matn::npos; i = k.find(U"dish", i + 1)) {
        if (i > 0 && ascii_harf(k[i - 1])) continue;
        Matn qolgan = k.substr(i + 4);
        if (qolgan.rfind(U"ormon", 0) == 0 || qolgan.rfind(U"idro", 0) == 0 || qolgan.rfind(U"armon", 0) == 0)
            return true;
    }
    return false;
}

}  // namespace

// -- belgi turlari --

bool harfmi(char32_t c) {
    if (c == 0 || ichida(APOSTROF, c)) return false;
    return ascii_harf(c) || c == 0xAA || c == 0xB5 || c == 0xBA || (c >= 0xC0 && c <= 0x24F && c != 0xD7 && c != 0xF7) ||
           (c >= 0x370 && c <= 0x3FF) || (c >= 0x400 && c <= 0x481) || (c >= 0x48A && c <= 0x52F);
}

bool boshliqmi(char32_t c) {  // Python str.isspace()
    return (c >= 0x09 && c <= 0x0D) || (c >= 0x1C && c <= 0x20) || c == 0x85 || c == 0xA0 || c == 0x1680 ||
           (c >= 0x2000 && c <= 0x200A) || c == 0x2028 || c == 0x2029 || c == 0x202F || c == 0x205F || c == 0x3000;
}

char32_t kichik(char32_t c) {
    if (c >= U'A' && c <= U'Z') return c + 32;
    if ((c >= 0xC0 && c <= 0xDE && c != 0xD7)) return c + 32;
    if (c == 0x178) return 0xFF;
    if (c >= 0x100 && c <= 0x17F) {
        bool juft_katta = (c <= 0x137) || (c >= 0x14A && c <= 0x177);
        bool toq_katta = (c >= 0x139 && c <= 0x148) || (c >= 0x179 && c <= 0x17E);
        if (c == 0x130) return U'i';
        if (juft_katta && c % 2 == 0) return c + 1;
        if (toq_katta && c % 2 == 1) return c + 1;
        return c;
    }
    if (c >= 0x410 && c <= 0x42F) return c + 0x20;
    if (c >= 0x400 && c <= 0x40F) return c + 0x50;
    if ((c >= 0x460 && c <= 0x481) || (c >= 0x48A && c <= 0x4BF) || (c >= 0x4D0 && c <= 0x52F))
        return c % 2 == 0 ? c + 1 : c;
    if (c >= 0x4C1 && c <= 0x4CE) return c % 2 == 1 ? c + 1 : c;
    if (c == 0x4C0) return 0x4CF;
    if (c >= 0x391 && c <= 0x3AB && c != 0x3A2) return c + 0x20;
    return c;
}

char32_t katta(char32_t c) {
    if (c >= U'a' && c <= U'z') return c - 32;
    if (c >= 0xE0 && c <= 0xFE && c != 0xF7) return c - 32;
    if (c == 0xFF) return 0x178;
    if (c >= 0x100 && c <= 0x17F) {
        bool juft_katta = (c <= 0x137) || (c >= 0x14A && c <= 0x177);
        bool toq_katta = (c >= 0x139 && c <= 0x148) || (c >= 0x179 && c <= 0x17E);
        if (c == 0x131) return U'I';
        if (juft_katta && c % 2 == 1 && c != 0x131) return c - 1;
        if (toq_katta && c % 2 == 0) return c - 1;
        return c;
    }
    if (c >= 0x430 && c <= 0x44F) return c - 0x20;
    if (c >= 0x450 && c <= 0x45F) return c - 0x50;
    if ((c >= 0x460 && c <= 0x481) || (c >= 0x48A && c <= 0x4BF) || (c >= 0x4D0 && c <= 0x52F))
        return c % 2 == 1 ? c - 1 : c;
    if (c >= 0x4C2 && c <= 0x4CE) return c % 2 == 0 ? c - 1 : c;
    if (c == 0x4CF) return 0x4C0;
    if (c >= 0x3B1 && c <= 0x3CB && c != 0x3C2) return c - 0x20;
    return c;
}

Matn kichik(const Matn& m) {
    Matn r = m;
    for (auto& c : r) c = kichik(c);
    return r;
}
Matn katta(const Matn& m) {
    Matn r = m;
    for (auto& c : r) c = katta(c);
    return r;
}

bool havolami(const Matn& s) {
    if (s.find(U"://") != Matn::npos || ichida(s, U'@')) return true;
    if (s.size() >= 4 && kichik(s.substr(0, 4)) == U"www.") return true;
    for (size_t i = 0; i < s.size(); ++i) {
        if (ichida(NOTANISH, kichik(s[i])) || s[i] == 0x1E9E) return true;
        if (i + 1 < s.size() && ((ascii_harf(s[i]) && raqam(s[i + 1])) || (raqam(s[i]) && ascii_harf(s[i + 1]))))
            return true;
        if (i + 3 < s.size() && ascii_harf(s[i]) && s[i + 1] == U'.' && ascii_harf(s[i + 2]) && ascii_harf(s[i + 3]))
            return true;
    }
    return false;
}

bool xom_qolsinmi(const Matn& soz, bool yakuniy) {
    if (havolami(soz) || xorijiy(soz)) return true;
    if (!yakuniy) return false;
    Matn r = ong_tozala(soz, TINISH);
    if (!r.empty() && (r.back() == U'c' || r.back() == U'C')) return true;
    return qisqartmami(soz) || dis_h(soz);
}

// -- lug'at --

void Lugat::qosh(const Matn& kirgan) {
    size_t a = 0, b = kirgan.size();
    while (a < b && boshliqmi(kirgan[a])) ++a;
    while (b > a && boshliqmi(kirgan[b - 1])) --b;
    Matn soz = kirgan.substr(a, b - a);
    if (soz.size() < 2) return;
    kalit_[kichik(soz)] = soz;
    kalit_[kichik(sof_ogir(soz))] = soz;
}

size_t Lugat::hajm() const {
    std::unordered_set<Matn> turli;
    for (const auto& [k, v] : kalit_) turli.insert(v);
    return turli.size();
}

std::optional<Matn> Lugat::top(const Matn& soz) const {
    if (soz.size() < 2) return std::nullopt;
    Matn k = kichik(soz);
    if (auto it = kalit_.find(k); it != kalit_.end()) return harf_holati(it->second, soz);
    // qo'shimchali shakl: Chromeda, Shellga (qisqa so'zda emas — bashorat ≠ bash+orat)
    long chegara = std::max<long>(4, static_cast<long>(k.size()) - 6);
    for (long n = static_cast<long>(k.size()) - 1; n > chegara; --n) {
        auto it = kalit_.find(k.substr(0, n));
        if (it != kalit_.end() && n >= 5) return harf_holati(it->second, soz.substr(0, n)) + soz.substr(n);
    }
    return std::nullopt;
}

std::optional<Matn> Lugat::tuzat(const Matn& token) const {
    size_t bosh = token.size() - chap_tozala(token, TINISH).size();
    size_t oxir = ong_tozala(token, TINISH).size();
    if (oxir <= bosh || oxir - bosh < 2) return std::nullopt;
    Matn soz = token.substr(bosh, oxir - bosh);
    std::optional<Matn> yangi = top(soz);
    if (!yangi && ichida(soz, U'-')) {  // Arnold-Chiari, four-channel
        Matn natija, qism;
        for (char32_t c : soz + U"-") {
            if (c == U'-') {
                auto t = top(qism);
                natija += (t ? *t : qism) + U"-";
                qism.clear();
            } else {
                qism += c;
            }
        }
        natija.pop_back();
        yangi = natija;
    }
    if (!yangi || *yangi == soz) return std::nullopt;
    return token.substr(0, bosh) + *yangi + token.substr(oxir);
}

std::vector<Matn> lugat_qatorlari(const std::string& fayl_matni) {
    std::vector<Matn> sozlar;
    Matn hammasi = utf8_dan(fayl_matni), qator;
    hammasi += U'\n';
    for (char32_t c : hammasi) {
        if (c != U'\n') {
            qator += c;
            continue;
        }
        if (auto i = qator.find(U'#'); i != Matn::npos) qator.resize(i);
        size_t a = 0, b = qator.size();
        while (a < b && boshliqmi(qator[a])) ++a;
        while (b > a && boshliqmi(qator[b - 1])) --b;
        if (b > a) sozlar.push_back(qator.substr(a, b - a));
        qator.clear();
    }
    return sozlar;
}

// -- o'giruvchi --

void Ogirgich::token_yangi() {
    token_xom.clear();
    token_tugma.clear();
    token_chiq.clear();
    otkaz = buzildi = false;
}

Matn Ogirgich::chiqar(const Matn& m) {
    if (!m.empty()) {
        oldingi = m.back();
        oldingi_nomalum = false;
        yumshoq_ = false;
        token_chiq += m;
    }
    return m;
}

std::optional<Matn> Ogirgich::asl_nusxa(bool yakuniy) const {
    if (xom_qolsinmi(token_xom, yakuniy)) return token_xom;
    if (havolami(token_tugma)) return token_tugma;
    return std::nullopt;
}

void Ogirgich::yakunla() {
    tugat();
    if (buzildi || !s.istisnolar) return;
    const Matn xom = token_xom;
    const bool lotin_soz = !kirill_bor(xom);
    const bool lugat_bor = lugat && !lugat->bosh();
    if (lugat_bor && lotin_soz) {  // lug'atdagi butun nom: Shy-Drager, MACH-NC
        if (auto butun = lugat->tuzat(token_chiq)) {
            token_chiq = *butun;
            return;
        }
    }
    Matn ichki = tozala(xom, TINISH);
    bool ajratgich = false;
    for (char32_t c : ichki) ajratgich = ajratgich || ichida(QISM_AJRATGICH, c);
    if (ajratgich && !havolami(token_tugma) && lotin_soz) {  // C-uchli, MR-shartli
        Matn natija, bolak;
        for (char32_t c : xom) {
            if (ichida(QISM_AJRATGICH, c)) {
                natija += qism(bolak);
                natija += c;
                bolak.clear();
            } else {
                bolak += c;
            }
        }
        natija += qism(bolak);
        token_chiq = natija;
        return;
    }
    if (otkaz) return;
    if (auto asl = asl_nusxa(true)) {
        token_chiq = *asl;
    } else if (lugat_bor && !kirill_bor(token_xom)) {
        if (auto t = lugat->tuzat(token_chiq)) token_chiq = *t;
    }
}

Matn Ogirgich::qism(const Matn& q) const {
    if (q.empty() || xom_qolsinmi(q, true)) return q;
    Sozlama toza = s;
    toza.istisnolar = false;
    Ogirgich og(toza);
    Matn chiq;
    for (char32_t c : q) chiq += og.qabul(c);
    chiq += og.tugat();
    if (lugat && !lugat->bosh())
        if (auto t = lugat->tuzat(chiq)) return *t;
    return chiq;
}

Matn Ogirgich::qabul(char32_t c, char32_t tugma) {
    if (boshliqmi(c)) {
        Matn m = tugat();
        m += chiqar(c);
        token_yangi();
        return m;
    }
    token_xom += c;
    token_tugma += tugma ? tugma : c;
    if (s.istisnolar && !otkaz) {
        if (auto asl = asl_nusxa(false)) {
            otkaz = true;
            Matn m = tugat();
            m += chiqar(tugma ? tugma : c);
            token_chiq = *asl;  // so'z boshidan asliga qaytadi
            return m;
        }
    }
    if (otkaz) return chiqar(tugma ? tugma : c);
    if (kutish.empty()) return yangi(c);
    Matn m;
    bool yutdi = false;
    if (!davom(c, m, yutdi)) {
        Matn t = tugat();
        t += yangi(c);
        return t;
    }
    if (!yutdi) m += yangi(c);
    return m;
}

Matn Ogirgich::tugat() {
    Matn k = kutish;
    kutish.clear();
    if (k.empty()) return {};
    if (k[0] == KATTA_KIRILL) return chiqar(bosh_harf(k.substr(1)));
    return chiqar(k);
}

Matn Ogirgich::korinish() const {
    if (!kutish.empty() && kutish[0] == KATTA_KIRILL) return bosh_harf(kutish.substr(1));
    return kutish;
}

void Ogirgich::orqaga() {
    if (kutish.empty() || kutish[0] == KATTA_KIRILL) kutish.clear();
    else kutish.pop_back();
    if (!token_xom.empty()) token_xom.pop_back();
}

void Ogirgich::ochirildi() {
    if (!token_chiq.empty()) token_chiq.pop_back();
    if (!token_xom.empty()) token_xom.pop_back();
    buzildi = true;
}

Matn Ogirgich::xom(char32_t c) {
    Matn m = tugat();
    token_xom += c;
    token_tugma += c;
    m += chiqar(c);
    if (s.istisnolar && !otkaz && xom_qolsinmi(token_xom)) {
        otkaz = true;
        token_chiq = token_xom;
    }
    return m;
}

void Ogirgich::unut() {
    kutish.clear();
    oldingi = 0;
    oldingi_nomalum = true;
    yumshoq_ = false;
    token_yangi();
}

Matn Ogirgich::yangi(char32_t c) {
    if (s.lotin && ichida(LOTIN_BOSH, c)) {
        kutish = Matn(1, c);
        return {};
    }
    if (s.tutuq && (c == U'\'' || c == U'’') && harfmi(oldingi)) {
        kutish = U"'";
        return {};
    }
    if (s.kirill) {
        char32_t past = kichik(c);
        if (kirill_lotin(past) || ichida(KIRILL_ORINGA_QARAB, past)) return kirill(c, past);
    }
    return chiqar(c);
}

bool Ogirgich::davom(char32_t c, Matn& matn, bool& yutdi) {
    const Matn k = kutish;
    const bool h = c == U'h' || c == U'H';
    if (k[0] == KATTA_KIRILL) {
        kutish.clear();
        Matn lat = k.substr(1);
        matn = chiqar(kattami(c) ? katta(lat) : bosh_harf(lat));
        yutdi = false;
        return true;
    }
    if (k == U"'") {
        kutish.clear();
        matn = chiqar(harfmi(c) ? Matn(1, TUTUQ) : Matn(U"'"));
        yutdi = false;
        return true;
    }
    if ((k == U"s" || k == U"S" || k == U"c" || k == U"C") && h) {
        kutish.clear();
        matn = chiqar(k == U"s" ? U"ş" : k == U"S" ? U"Ş" : k == U"c" ? U"ç" : U"Ç");
        yutdi = true;
        return true;
    }
    if ((k == U"s" || k == U"S") && ichida(APOSTROF, c) && s.sh_ajrat) {
        kutish = k + c;
        matn.clear();
        yutdi = true;
        return true;
    }
    if (k.size() == 2) {  // s' dan keyin
        kutish.clear();
        if (h) {
            matn = chiqar(Matn{k[0], c});
            yutdi = true;
            return true;
        }
        char32_t apostrof = (s.tutuq && harfmi(c)) ? TUTUQ : k[1];
        matn = chiqar(Matn{k[0], apostrof});
        yutdi = false;
        return true;
    }
    if ((k == U"o" || k == U"O" || k == U"g" || k == U"G") && ichida(APOSTROF, c)) {
        kutish.clear();
        matn = chiqar(k == U"o" ? U"ö" : k == U"O" ? U"Ö" : k == U"g" ? U"ğ" : U"Ğ");
        yutdi = true;
        return true;
    }
    return false;
}

Matn Ogirgich::kirill(char32_t c, char32_t past) {
    if (past == U'ь') {
        yumshoq_ = true;
        return {};
    }
    if (past == U'ъ') return chiqar(s.tutuq ? Matn(1, TUTUQ) : Matn(U"'"));
    Matn lat;
    if (past == U'е') {
        char32_t o = oldingi;
        lat = (yumshoq_ || !harfmi(o) || ichida(UNLI, o)) ? U"ye" : U"e";
    } else if (past == U'ц') {
        lat = ichida(UNLI, oldingi) ? U"ts" : U"s";
    } else {
        lat = *kirill_lotin(past);
    }
    if (c == past) return chiqar(lat);
    if (lat.size() == 1 || (harfmi(oldingi) && kattami(oldingi))) return chiqar(katta(lat));
    kutish = Matn(1, KATTA_KIRILL) + lat;
    return {};
}

// -- butun matn --

Matn sof_ogir(const Matn& matn) {
    Sozlama s;
    s.istisnolar = false;
    Ogirgich og(s);
    Matn r;
    for (char32_t c : matn) r += og.qabul(c);
    r += og.tugat();
    return r;
}

Matn ogir(const Matn& matn, Sozlama s, const Lugat* lugat) {
    Ogirgich og(s, lugat);
    Matn r;
    for (char32_t c : matn) {
        if (boshliqmi(c)) {
            og.yakunla();
            r += og.token_chiq;
            og.qabul(c);
            r += c;
        } else {
            og.qabul(c);
        }
    }
    og.yakunla();
    r += og.token_chiq;
    return r;
}

Matn eski_yozuvda(const Matn& matn) {
    Matn r;
    for (size_t i = 0; i < matn.size(); ++i) {
        char32_t c = matn[i], past = kichik(c);
        Matn eski = past == U'ş' ? U"sh" : past == U'ç' ? U"ch" : past == U'ö' ? U"o'" : past == U'ğ' ? U"g'" : U"";
        if (eski.empty()) {
            r += c;
            continue;
        }
        if (c != past) {
            bool keyingi_katta = i + 1 < matn.size() && kattami(Matn(1, matn[i + 1]));
            eski = keyingi_katta ? katta(eski) : Matn(1, katta(eski[0])) + eski.substr(1);
        }
        r += eski;
    }
    return r;
}

// -- kodlash --

Matn utf8_dan(const std::string& s) {
    Matn r;
    for (size_t i = 0; i < s.size();) {
        unsigned char b = s[i];
        char32_t c;
        int n;
        if (b < 0x80) { c = b; n = 1; }
        else if ((b >> 5) == 6) { c = b & 0x1F; n = 2; }
        else if ((b >> 4) == 14) { c = b & 0x0F; n = 3; }
        else { c = b & 0x07; n = 4; }
        for (int j = 1; j < n && i + j < s.size(); ++j) c = (c << 6) | (static_cast<unsigned char>(s[i + j]) & 0x3F);
        r += c;
        i += n;
    }
    return r;
}

std::string utf8_ga(const Matn& m) {
    std::string r;
    for (char32_t c : m) {
        if (c < 0x80) r += static_cast<char>(c);
        else if (c < 0x800) { r += static_cast<char>(0xC0 | (c >> 6)); r += static_cast<char>(0x80 | (c & 0x3F)); }
        else if (c < 0x10000) {
            r += static_cast<char>(0xE0 | (c >> 12));
            r += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            r += static_cast<char>(0x80 | (c & 0x3F));
        } else {
            r += static_cast<char>(0xF0 | (c >> 18));
            r += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
            r += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            r += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    return r;
}

Matn utf16_dan(const std::u16string& s) {
    Matn r;
    for (size_t i = 0; i < s.size(); ++i) {
        char32_t c = s[i];
        if (c >= 0xD800 && c <= 0xDBFF && i + 1 < s.size()) c = 0x10000 + ((c - 0xD800) << 10) + (s[++i] - 0xDC00);
        r += c;
    }
    return r;
}

std::u16string utf16_ga(const Matn& m) {
    std::u16string r;
    for (char32_t c : m) {
        if (c < 0x10000) r += static_cast<char16_t>(c);
        else {
            c -= 0x10000;
            r += static_cast<char16_t>(0xD800 + (c >> 10));
            r += static_cast<char16_t>(0xDC00 + (c & 0x3FF));
        }
    }
    return r;
}

}  // namespace ozbek
