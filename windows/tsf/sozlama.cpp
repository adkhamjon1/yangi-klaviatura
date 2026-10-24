// Sozlamalar (HKCU\Software\YangiKlaviatura) va foydalanuvchi lug'ati (%APPDATA%\YangiKlaviatura\lugat.txt).
#include <shlobj.h>

#include <fstream>
#include <sstream>

#include "lugat.inc"
#include "umumiy.hpp"

namespace xizmat {
namespace {

bool qiymat(HKEY kalit, const wchar_t* nom, bool standart) {
    DWORD v = 0, tur = 0, hajm = sizeof v;
    if (RegQueryValueExW(kalit, nom, nullptr, &tur, reinterpret_cast<BYTE*>(&v), &hajm) == ERROR_SUCCESS && tur == REG_DWORD)
        return v != 0;
    return standart;
}

std::wstring lugat_fayli() { return papka(CSIDL_APPDATA) + L"\\lugat.txt"; }

}  // namespace

std::wstring papka(int csidl_turi) {
    wchar_t yol[MAX_PATH] = L"";
    SHGetFolderPathW(nullptr, csidl_turi | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, yol);
    std::wstring p = std::wstring(yol) + L"\\YangiKlaviatura";
    CreateDirectoryW(p.c_str(), nullptr);
    return p;
}

WinSozlama sozlama_oqi() {
    WinSozlama w;
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REESTR, 0, KEY_READ, &k) != ERROR_SUCCESS) return w;
    auto& d = w.dvigatel;
    d.tugmalar = qiymat(k, L"tugmalar", true);
    d.ogir.lotin = qiymat(k, L"lotin", true);
    d.ogir.kirill = qiymat(k, L"kirill", true);
    d.ogir.sh_ajrat = qiymat(k, L"sh_ajrat", true);
    d.ogir.tutuq = qiymat(k, L"tutuq", false);
    d.ogir.istisnolar = qiymat(k, L"istisno", true);
    d.lugat = qiymat(k, L"lugat", true);
    d.organish = qiymat(k, L"organish", true);
    d.eski_belgi = qiymat(k, L"eski_belgi", false);
    w.darhol = qiymat(k, L"darhol", true);
    RegCloseKey(k);
    return w;
}

Kuzatuvchi::Kuzatuvchi() {
    RegCreateKeyExW(HKEY_CURRENT_USER, REESTR, 0, nullptr, 0, KEY_NOTIFY | KEY_READ, nullptr, &kalit_, nullptr);
    hodisa_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    qayta_qur();
    lugat_vaqti_ = vaqt();
}

Kuzatuvchi::~Kuzatuvchi() {
    if (kalit_) RegCloseKey(kalit_);
    if (hodisa_) CloseHandle(hodisa_);
}

void Kuzatuvchi::qayta_qur() {
    if (kalit_ && hodisa_)
        RegNotifyChangeKeyValue(kalit_, FALSE, REG_NOTIFY_CHANGE_LAST_SET | 0x10000000L /* THREAD_AGNOSTIC */,
                                hodisa_, TRUE);
}

ULONGLONG Kuzatuvchi::vaqt() {
    WIN32_FILE_ATTRIBUTE_DATA d;
    if (!GetFileAttributesExW(lugat_fayli().c_str(), GetFileExInfoStandard, &d)) return 0;
    return (static_cast<ULONGLONG>(d.ftLastWriteTime.dwHighDateTime) << 32) | d.ftLastWriteTime.dwLowDateTime;
}

bool Kuzatuvchi::ozgardimi() {
    bool ozgardi = false;
    if (hodisa_ && WaitForSingleObject(hodisa_, 0) == WAIT_OBJECT_0) {
        ozgardi = true;
        qayta_qur();
    }
    ULONGLONG v = vaqt();
    if (v != lugat_vaqti_) {
        lugat_vaqti_ = v;
        ozgardi = true;
    }
    return ozgardi;
}

ozbek::Lugat lugat_yasa() {
    ozbek::Lugat l;
    for (const auto& soz : ozbek::lugat_qatorlari(ORNATILGAN_LUGAT)) l.qosh(soz);
    std::ifstream f(lugat_fayli().c_str(), std::ios::binary);
    if (f) {
        std::stringstream ss;
        ss << f.rdbuf();
        for (const auto& soz : ozbek::lugat_qatorlari(ss.str())) l.qosh(soz);
    }
    return l;
}

void lugatga_qosh(const ozbek::Matn& soz) {
    std::ifstream oqish(lugat_fayli().c_str(), std::ios::binary);
    std::stringstream ss;
    if (oqish) ss << oqish.rdbuf();
    for (const auto& bor : ozbek::lugat_qatorlari(ss.str()))
        if (ozbek::kichik(bor) == ozbek::kichik(soz)) return;
    std::ofstream f(lugat_fayli().c_str(), std::ios::binary | std::ios::app);
    f << ozbek::utf8_ga(soz) << '\n';
}

}  // namespace xizmat
