// TSF xizmati uchun umumiy e'lonlar: GUID'lar, modul hisoblagichi, sozlamalar.
#pragma once

#include <windows.h>
#include <msctf.h>

#include <string>

#include "../yadro/dvigatel.hpp"

namespace xizmat {

// {1b5150a6-39e7-42b1-b771-bbf2a214010b} — matn xizmati (COM sinfi)
constexpr CLSID CLSID_OZBEK = {0x1b5150a6, 0x39e7, 0x42b1, {0xb7, 0x71, 0xbb, 0xf2, 0xa2, 0x14, 0x01, 0x0b}};
// Profillar: «öz», «o'z», «ўз»
constexpr GUID PROFIL_OZ = {0x1090cf6a, 0x22fc, 0x486d, {0xa2, 0xc4, 0x56, 0x6b, 0xfb, 0x57, 0x56, 0x7f}};
constexpr GUID PROFIL_ESKI = {0x45a97dd2, 0x5ad2, 0x40c8, {0x9e, 0x36, 0xa6, 0x4c, 0x28, 0x60, 0xc9, 0xf5}};
constexpr GUID PROFIL_KIRILL = {0x064d6f63, 0x6c7f, 0x4ea4, {0xaf, 0x76, 0xe4, 0xc8, 0xff, 0x0b, 0x21, 0x65}};

// MinGW sarlavhalarida yo'q TSF toifalari (Windows SDK msctf.h dagi qiymatlar)
constexpr GUID TOIFA_IMMERSIVE = {0x13A016DF, 0x560B, 0x46CD, {0x94, 0x7A, 0x4C, 0x3A, 0xF1, 0xE0, 0xE3, 0x5D}};
constexpr GUID TOIFA_SYSTRAY = {0x25504FB4, 0x7BAB, 0x4BC1, {0x9C, 0x69, 0xCF, 0x81, 0x89, 0x0F, 0x0E, 0xF5}};

constexpr LANGID TIL = MAKELANGID(LANG_UZBEK, SUBLANG_UZBEK_LATIN);  // 0x0443 uz-Latn-UZ
constexpr wchar_t NOM[] = L"Özbek klaviaturasi";
constexpr wchar_t REESTR[] = L"Software\\YangiKlaviatura";            // HKCU — sozlamalar

extern HINSTANCE modul;
extern LONG havolalar;  // DllCanUnloadNow uchun
inline void qosh() { InterlockedIncrement(&havolalar); }
inline void ayir() { InterlockedDecrement(&havolalar); }

// Sozlamalar (HKCU\Software\YangiKlaviatura, DWORD 0/1) va lug'at (%APPDATA%\YangiKlaviatura\lugat.txt)
struct WinSozlama {
    ozbek::DvigatelSozlama dvigatel;
    bool darhol = true;
    bool jurnal = false;
};
WinSozlama sozlama_oqi();
bool sozlama_ber(const wchar_t* kalit);     // bitta toggle (yozilmagan bo'lsa standart qiymat)
bool sozlama_yoz(const wchar_t* kalit, bool qiymat);
std::wstring modul_papkasi();               // DLL turgan papka (Sozlamalar.exe ham shu yerda)
void jurnal_yoz(const std::wstring& qator); // faqat sozlamada «jurnal» yoqilganda chaqiriladi
ozbek::Lugat lugat_yasa();                  // o'rnatilgan + foydalanuvchiniki
void lugatga_qosh(const ozbek::Matn& soz);
std::wstring papka(int csidl_turi);         // masalan %APPDATA%\YangiKlaviatura (papka yaratiladi)

// Reestr va foydalanuvchi lug'atidagi o'zgarishni sezadi (har xizmat nusxasida bittadan)
class Kuzatuvchi {
public:
    Kuzatuvchi();
    ~Kuzatuvchi();
    Kuzatuvchi(const Kuzatuvchi&) = delete;
    Kuzatuvchi& operator=(const Kuzatuvchi&) = delete;
    bool ozgardimi();

private:
    void qayta_qur();
    static ULONGLONG vaqt();
    HKEY kalit_ = nullptr;
    HANDLE hodisa_ = nullptr;
    ULONGLONG lugat_vaqti_ = 0;
};

}  // namespace xizmat
