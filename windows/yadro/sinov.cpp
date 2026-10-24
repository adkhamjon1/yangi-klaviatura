// C++ yadro sinovi:
//   sinov HOLATLAR.tsv LUGAT.txt     — umumiy holatlar (Python bilan bir xil)
//   sinov --oqim LUGAT.txt < kirish   — har qator o'giriladi (korpusni Python bilan solishtirish uchun)
#include <fstream>
#include <iostream>
#include <sstream>

#include "ogirgich.hpp"

using namespace ozbek;

static std::string fayl(const std::string& yol) {
    std::ifstream f(yol, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static Lugat lugat_yasa(const std::vector<Matn>& sozlar) {
    Lugat l;
    for (const auto& s : sozlar) l.qosh(s);
    return l;
}

static std::vector<std::string> bol(const std::string& s, char ajratgich) {
    std::vector<std::string> qismlar;
    std::string q;
    for (char c : s + ajratgich) {
        if (c == ajratgich) {
            qismlar.push_back(q);
            q.clear();
        } else {
            q += c;
        }
    }
    return qismlar;
}

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--oqim") {
        Lugat l = lugat_yasa(lugat_qatorlari(fayl(argv[2])));
        std::string q;
        while (std::getline(std::cin, q)) std::cout << utf8_ga(ogir(utf8_dan(q), {}, &l)) << '\n';
        return 0;
    }
    if (argc != 3) {
        std::cerr << "sinov HOLATLAR.tsv LUGAT.txt | sinov --oqim LUGAT.txt\n";
        return 2;
    }
    const Lugat ornatilgan = lugat_yasa(lugat_qatorlari(fayl(argv[2])));
    int jami = 0, xato = 0;
    auto tekshir = [&](const std::string& nom, const Matn& natija, const Matn& kutilgan) {
        ++jami;
        if (natija != kutilgan) {
            ++xato;
            std::cout << "✗ " << nom << " → «" << utf8_ga(natija) << "» (kutilgan «" << utf8_ga(kutilgan) << "»)\n";
        }
    };

    for (const auto& qator : bol(fayl(argv[1]), '\n')) {
        if (qator.empty() || qator[0] == '#') continue;
        auto ustun = bol(qator, '\t');
        if (ustun.size() != 3) continue;
        Sozlama s;
        Lugat ozi;
        const Lugat* lugat = nullptr;
        for (const auto& b : ustun[0] == "-" ? std::vector<std::string>{} : bol(ustun[0], ',')) {
            if (b == "L") lugat = &ornatilgan;
            else if (b.rfind("lugat=", 0) == 0) {
                for (const auto& soz : bol(b.substr(6), '|')) ozi.qosh(utf8_dan(soz));
                lugat = &ozi;
            } else if (b == "tutuq") s.tutuq = true;
            else if (b == "-lotin") s.lotin = false;
            else if (b == "-kirill") s.kirill = false;
            else if (b == "-sh") s.sh_ajrat = false;
            else if (b == "-istisno") s.istisnolar = false;
        }
        tekshir(ustun[0] + " " + ustun[1], ogir(utf8_dan(ustun[1]), s, lugat), utf8_dan(ustun[2]));
    }

    // eski yozuvga qaytarish (Backspace uchun)
    tekshir("eski Çrome", eski_yozuvda(U"Çrome"), U"Chrome");
    tekshir("eski ŞAHAR", eski_yozuvda(U"ŞAHAR"), U"SHAHAR");
    tekshir("eski Özbekiston", eski_yozuvda(U"Özbekiston"), U"O'zbekiston");
    // oqim: dvigatel ko'radigan holat
    {
        Ogirgich og;
        og.qabul(U's');
        tekshir("oqim s korinish", og.korinish(), U"s");
        tekshir("oqim sh", og.qabul(U'h'), U"ş");
        og.qabul(U' ');
        for (char32_t c : Matn(U"shifo.uz")) og.qabul(c);
        tekshir("oqim shifo.uz ekran", og.ekran(), U"shifo.uz");
        Ogirgich og2;
        og2.qabul(U's');
        tekshir("oqim AltGr", og2.xom(U'w'), U"sw");
        Ogirgich og3;
        og3.unut();
        tekshir("oqim noma'lum е", og3.qabul(U'е'), U"ye");
    }
    tekshir("lug'at hajmi > 150", Matn(ornatilgan.hajm() > 150 ? U"ha" : U"yo'q"), U"ha");

    std::cout << (xato ? "✗ " : "✓ ") << jami - xato << "/" << jami << " o'tdi\n";
    return xato ? 1 : 0;
}
