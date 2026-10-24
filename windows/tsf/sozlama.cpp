// Sozlamalar (HKCU\Software\YangiKlaviatura) va foydalanuvchi lug'ati (%APPDATA%\YangiKlaviatura\lugat.txt).
#include <shlobj.h>

#include <fstream>
#include <sstream>

#include "lugat.inc"
#include "umumiy.hpp"

namespace xizmat {
namespace {

// Standart qiymatlar — oyna/Sozlamalar.cs va linux/ibus/sozlama.py STANDART bilan bir xil
struct Standart {
    const wchar_t* kalit;
    bool qiymat;
};
constexpr Standart STANDART[] = {
    {L"tugmalar", true}, {L"lotin", true},      {L"kirill", true},  {L"sh_ajrat", true},    {L"tutuq", false},
    {L"istisno", true},  {L"lugat", true},      {L"organish", true}, {L"eski_belgi", false}, {L"darhol", true},
    {L"jurnal", false},  // tashxis: har tugma qarori %LOCALAPPDATA%\YangiKlaviatura\jurnal.txt ga
};

bool standart(const wchar_t* nom) {
    for (const auto& s : STANDART)
        if (!wcscmp(s.kalit, nom)) return s.qiymat;
    return false;
}

bool qiymat(HKEY kalit, const wchar_t* nom) {
    DWORD v = 0, tur = 0, hajm = sizeof v;
    if (RegQueryValueExW(kalit, nom, nullptr, &tur, reinterpret_cast<BYTE*>(&v), &hajm) == ERROR_SUCCESS && tur == REG_DWORD)
        return v != 0;
    return standart(nom);
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
    d.tugmalar = qiymat(k, L"tugmalar");
    d.ogir.lotin = qiymat(k, L"lotin");
    d.ogir.kirill = qiymat(k, L"kirill");
    d.ogir.sh_ajrat = qiymat(k, L"sh_ajrat");
    d.ogir.tutuq = qiymat(k, L"tutuq");
    d.ogir.istisnolar = qiymat(k, L"istisno");
    d.lugat = qiymat(k, L"lugat");
    d.organish = qiymat(k, L"organish");
    d.eski_belgi = qiymat(k, L"eski_belgi");
    w.darhol = qiymat(k, L"darhol");
    w.jurnal = qiymat(k, L"jurnal");
    RegCloseKey(k);
    return w;
}

void jurnal_yoz(const std::wstring& qator) {
    static const std::wstring fayl = papka(CSIDL_LOCAL_APPDATA) + L"\\jurnal.txt";
    wchar_t nom[MAX_PATH] = L"";
    GetModuleFileNameW(nullptr, nom, MAX_PATH);
    const wchar_t* qisqa = wcsrchr(nom, L'\\');
    SYSTEMTIME t;
    GetLocalTime(&t);
    wchar_t bosh[96];
    swprintf(bosh, 96, L"%02d:%02d:%02d.%03d %ls t%lu ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds,
             qisqa ? qisqa + 1 : nom, GetCurrentThreadId());
    const std::wstring s = bosh + qator + L"\n";
    std::ofstream f(fayl.c_str(), std::ios::binary | std::ios::app);
    f << ozbek::utf8_ga(ozbek::utf16_dan(std::u16string(s.begin(), s.end())));
}

bool sozlama_ber(const wchar_t* nom) {
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REESTR, 0, KEY_READ, &k) != ERROR_SUCCESS) return standart(nom);
    bool v = qiymat(k, nom);
    RegCloseKey(k);
    return v;
}

bool sozlama_yoz(const wchar_t* nom, bool v) {
    HKEY k;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REESTR, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr) != ERROR_SUCCESS)
        return false;
    DWORD d = v ? 1 : 0;
    bool ok = RegSetValueExW(k, nom, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&d), sizeof d) == ERROR_SUCCESS;
    RegCloseKey(k);
    return ok;
}

std::wstring modul_papkasi() {
    wchar_t yol[MAX_PATH] = L"";
    GetModuleFileNameW(modul, yol, MAX_PATH);
    std::wstring p = yol;
    return p.substr(0, p.find_last_of(L'\\'));
}

Kuzatuvchi::Kuzatuvchi() {
    hodisa_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    qayta_qur();
    lugat_vaqti_ = vaqt();
}

Kuzatuvchi::~Kuzatuvchi() {
    if (kalit_) RegCloseKey(kalit_);
    if (hodisa_) CloseHandle(hodisa_);
}

// Kalit har safar qayta ochiladi: u o'chirilib qayta yaratilsa, eski handle hech qachon xabar bermaydi
void Kuzatuvchi::qayta_qur() {
    if (kalit_) RegCloseKey(kalit_);
    kalit_ = nullptr;
    RegCreateKeyExW(HKEY_CURRENT_USER, REESTR, 0, nullptr, 0, KEY_NOTIFY | KEY_READ, nullptr, &kalit_, nullptr);
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
