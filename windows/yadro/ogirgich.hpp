// Eski o'zbek yozuvini (lotin va kirill) yangi lotin alifbosiga o'giradi — C++ yadro.
// linux/ibus/ogirgich.py bilan bir xil qoidalar; ikkalasi sinov/holatlar.tsv bilan sinaladi.
#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ozbek {

using Matn = std::u32string;

struct Sozlama {
    bool lotin = true;       // sh → ş, ch → ç, o' → ö, g' → ğ
    bool kirill = true;      // kirill harflari → yangi lotin
    bool sh_ajrat = true;    // s'h → sh (Is'hoq → Ishoq)
    bool tutuq = false;      // so'z ichidagi ' → ʼ
    bool istisnolar = true;  // havola, email, xorijiy so'z o'girilmaydi
};

// Xorijiy so'zlar: o'girilgan ko'rinishi (Çrome) ham, asli (chrome) ham taniladi.
class Lugat {
public:
    void qosh(const Matn& soz);
    std::optional<Matn> tuzat(const Matn& token) const;   // o'zgarmasa — bo'sh
    size_t hajm() const;
    bool bosh() const { return kalit_.empty(); }

private:
    std::optional<Matn> top(const Matn& soz) const;
    std::unordered_map<Matn, Matn> kalit_;
};

class Ogirgich {
public:
    explicit Ogirgich(Sozlama s = {}, const Lugat* lugat = nullptr) : s(s), lugat(lugat) {}

    Sozlama s;
    const Lugat* lugat;

    Matn qabul(char32_t c, char32_t tugma = 0);  // tugma — to'g'ri tugmadagi harfning asli (ş → w)
    Matn tugat();
    Matn korinish() const;
    Matn ekran() const { return token_chiq + korinish(); }
    void yakunla();
    void orqaga();
    void ochirildi();
    Matn xom(char32_t c);
    void unut();

    // holat (dvigatel o'qiydi)
    Matn kutish;
    char32_t oldingi = 0;          // oxirgi chiqqan belgi; 0 — boshlanish
    bool oldingi_nomalum = false;  // kursor ko'chdi
    Matn token_xom, token_tugma, token_chiq;
    bool otkaz = false, buzildi = false;

private:
    bool yumshoq_ = false;
    void token_yangi();
    Matn chiqar(const Matn& m);
    Matn chiqar(char32_t c) { return chiqar(Matn(1, c)); }
    std::optional<Matn> asl_nusxa(bool yakuniy) const;
    Matn qism(const Matn& q) const;
    Matn yangi(char32_t c);
    bool davom(char32_t c, Matn& matn, bool& yutdi);
    Matn kirill(char32_t c, char32_t past);
};

Matn ogir(const Matn& matn, Sozlama s = {}, const Lugat* lugat = nullptr);
Matn sof_ogir(const Matn& matn);
Matn eski_yozuvda(const Matn& matn);
std::vector<Matn> lugat_qatorlari(const std::string& fayl_matni);  // # izoh, bo'sh qator tashlanadi

// belgi turlari
bool harfmi(char32_t c);
bool boshliqmi(char32_t c);
char32_t kichik(char32_t c);
char32_t katta(char32_t c);
Matn kichik(const Matn& m);
Matn katta(const Matn& m);
bool xom_qolsinmi(const Matn& soz, bool yakuniy = false);
bool havolami(const Matn& soz);

// kodlash
Matn utf8_dan(const std::string& s);
std::string utf8_ga(const Matn& m);
Matn utf16_dan(const std::u16string& s);
std::u16string utf16_ga(const Matn& m);

}  // namespace ozbek
