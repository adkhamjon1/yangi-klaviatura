// Tugma bosilishini matn o'zgarishiga aylantiradi — platformaga bog'liq emas.
// linux/ibus/engine.py mantig'ining nusxasi; Windows'da TSF qatlami javobni bajaradi,
// Linux'da sinov uni ilova matnini taqlid qiluvchi bufer bilan tekshiradi.
#pragma once

#include "ogirgich.hpp"

namespace ozbek {

enum class Rejim { Oz, Eski, Kirill };          // «öz», «o'z», «ўз»
enum class Maxsus { Yoq, Backspace, Enter, Tab, Kursor, Boshqa };

struct DvigatelSozlama {
    Sozlama ogir;               // lotin, kirill, sh_ajrat, tutuq, istisnolar
    bool tugmalar = true;       // W → ş, C → ç, [ → ö, ] → ğ
    bool lugat = true;
    bool organish = true;
    bool eski_belgi = false;    // «o'z» da o' → oʻ
};

struct Tugma {
    char32_t belgi = 0;     // foydalanuvchi bosgan belgi: «öz»/«o'z» da US harfi (w), «ўз» da kirill (ц)
    char32_t ilova = 0;     // tugma yeyilmasa ilova yozadigan belgi (US sxemasi bo'yicha)
    bool altgr = false;
    bool boshqaruv = false; // Ctrl, Alt, Win (AltGr emas)
    Maxsus maxsus = Maxsus::Yoq;
};

struct Javob {
    bool yedi = false;            // tugma ilovaga bormaydi
    // «darhol»: kursordan oldingi `oldingi` matni (uzunligi bo'yicha) o'chiriladi va `yoz` yoziladi;
    // bajaruvchi avval kursordan oldingi matn rostdan `oldingi` ekanini tekshiradi.
    Matn oldingi, yoz;
    // «tagiga» (kompozitsiya): `tasdiq` yoziladi, `kompozitsiya` tagiga chizilib turadi
    Matn tasdiq, kompozitsiya;
    bool kompozitsiya_bor = false;  // kompozitsiya yangilanishi kerak
    Matn organ;                   // foydalanuvchi lug'atiga yoziladigan so'z
    bool ozgarish() const { return !oldingi.empty() || !yoz.empty(); }
};

class Dvigatel {
public:
    Rejim rejim = Rejim::Oz;
    DvigatelSozlama s;
    const Lugat* lugat = nullptr;

    void sozla();                                   // s yoki lugat o'zgarganda
    Javob tugma(const Tugma& t, bool darhol);
    void unut();                                    // fokus, kursor ko'chdi
    void kompozitsiya_tugadi();                     // ilova kompozitsiyani o'zi yakunladi
    bool oldingi_kerak() const { return og_.oldingi_nomalum; }
    void oldingi_berildi(char32_t c) { og_.oldingi = c; og_.oldingi_nomalum = false; }
    const Ogirgich& ogirgich() const { return og_; }

private:
    Ogirgich og_;
    Matn ekranda_;                                  // «darhol»: joriy so'z ekranda
    bool c_tugmadan_ = false;
    struct Qaytarish { Matn ekrandagi, eski; };
    bool qaytarish_bor_ = false;
    Qaytarish qaytarish_;
    char32_t eski_oldingi_ = 0;

    Javob eski(const Tugma& t);
    Javob soz_oxiri(bool darhol, char32_t chegara, Javob j);
    Javob orqaga(bool darhol, bool qaytarish_bor, const Qaytarish& q);
    void tasdiqla(bool darhol, Javob& j);
    static void farq(const Matn& oldin, const Matn& yangi, Javob& j);
};

}  // namespace ozbek
