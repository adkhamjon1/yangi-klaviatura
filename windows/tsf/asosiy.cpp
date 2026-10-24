// DLL kirish nuqtalari: COM sinf fabrikasi va ro'yxatdan o'tkazish (regsvr32).
#include <new>
#include <string>

#include "resurs.h"
#include "umumiy.hpp"


namespace xizmat {

HINSTANCE modul = nullptr;
LONG havolalar = 0;
HRESULT xizmat_yarat(REFIID riid, void** ppv);

namespace {

class Fabrika final : public IClassFactory {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override {
        qosh();
        return 2;
    }
    STDMETHODIMP_(ULONG) Release() override {
        ayir();
        return 1;
    }
    STDMETHODIMP CreateInstance(IUnknown* tashqi, REFIID riid, void** ppv) override {
        if (!ppv) return E_INVALIDARG;
        *ppv = nullptr;
        if (tashqi) return CLASS_E_NOAGGREGATION;
        return xizmat_yarat(riid, ppv);
    }
    STDMETHODIMP LockServer(BOOL qulf) override {
        qulf ? qosh() : ayir();
        return S_OK;
    }
};

Fabrika fabrika;

std::wstring guid_matn(REFGUID g) {
    wchar_t buf[64];
    StringFromGUID2(g, buf, 64);
    return buf;
}

struct Profil {
    const GUID* guid;
    const wchar_t* nom;
    int ikonka;
};
const Profil PROFILLAR[] = {
    {&PROFIL_OZ, L"Özbek — yangi yozuv (öz)", IKONKA_OZ},
    {&PROFIL_ESKI, L"O'zbek — eski lotin (o'z)", IKONKA_ESKI},
    {&PROFIL_KIRILL, L"Ўзбек — kirilldan yangi lotinga (ўз)", IKONKA_KIRILL},
};
// O'rinbosar sxema: o'zbek (lotin) tili + US tugmalari (HKL 04090443). Windows uz-Latn qo'shilganda uni
// o'zi yaratadi (Keyboard Layout\Substitutes: 00000443 → 00000409). en-US ning 04090409 i bo'lsa,
// fokus almashganda Windows HKL bo'yicha en-US klaviaturasini tanlab, rejimni o'chirib qo'yardi.
constexpr ULONG_PTR ORINBOSAR_HKL = 0x04090443;
const GUID* TOIFALAR[] = {&GUID_TFCAT_TIP_KEYBOARD, &TOIFA_IMMERSIVE, &TOIFA_SYSTRAY};

std::wstring dll_yoli() {
    wchar_t yol[MAX_PATH];
    GetModuleFileNameW(modul, yol, MAX_PATH);
    return yol;
}

LONG kalit_yoz(HKEY ildiz, const std::wstring& yol, const wchar_t* nom, const std::wstring& qiymat) {
    HKEY k;
    LONG r = RegCreateKeyExW(ildiz, yol.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr);
    if (r != ERROR_SUCCESS) return r;
    r = RegSetValueExW(k, nom, 0, REG_SZ, reinterpret_cast<const BYTE*>(qiymat.c_str()),
                       static_cast<DWORD>((qiymat.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(k);
    return r;
}

HRESULT com_yoz() {
    const std::wstring yol = L"Software\\Classes\\CLSID\\" + guid_matn(CLSID_OZBEK);
    if (kalit_yoz(HKEY_LOCAL_MACHINE, yol, nullptr, NOM) != ERROR_SUCCESS) return E_FAIL;
    if (kalit_yoz(HKEY_LOCAL_MACHINE, yol + L"\\InprocServer32", nullptr, dll_yoli()) != ERROR_SUCCESS)
        return E_FAIL;
    if (kalit_yoz(HKEY_LOCAL_MACHINE, yol + L"\\InprocServer32", L"ThreadingModel", L"Apartment") != ERROR_SUCCESS)
        return E_FAIL;
    return S_OK;
}

HRESULT profillar_yoz(bool yoz) {
    ITfInputProcessorProfileMgr* mgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfInputProcessorProfileMgr, reinterpret_cast<void**>(&mgr));
    if (FAILED(hr)) return hr;
    const std::wstring yol = dll_yoli();
    for (const auto& p : PROFILLAR) {
        if (yoz) {
            // eski versiyadan qolgan profil yangi qiymatlar (o'rinbosar sxema) bilan qayta yoziladi;
            // til ro'yxatidagi yozuvni o'rnatuvchi keyin InstallLayoutOrTip bilan tiklaydi
            mgr->UnregisterProfile(CLSID_OZBEK, TIL, *p.guid, 0);
            hr = mgr->RegisterProfile(CLSID_OZBEK, TIL, *p.guid, p.nom, static_cast<ULONG>(wcslen(p.nom)), yol.c_str(),
                                      static_cast<ULONG>(yol.size()), static_cast<ULONG>(-p.ikonka),
                                      reinterpret_cast<HKL>(ORINBOSAR_HKL), 0, TRUE, 0);
            if (FAILED(hr)) break;
        } else {
            mgr->UnregisterProfile(CLSID_OZBEK, TIL, *p.guid, 0);
        }
    }
    mgr->Release();
    return hr;
}

HRESULT toifalar_yoz(bool yoz) {
    ITfCategoryMgr* tm = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr,
                                  reinterpret_cast<void**>(&tm));
    if (FAILED(hr)) return hr;
    for (const GUID* t : TOIFALAR) {
        hr = yoz ? tm->RegisterCategory(CLSID_OZBEK, *t, CLSID_OZBEK) : tm->UnregisterCategory(CLSID_OZBEK, *t, CLSID_OZBEK);
        if (FAILED(hr) && yoz) break;
    }
    tm->Release();
    return yoz ? hr : S_OK;
}

}  // namespace
}  // namespace xizmat

using namespace xizmat;

STDAPI DllUnregisterServer();

extern "C" BOOL WINAPI DllMain(HINSTANCE h, DWORD sabab, LPVOID) {
    if (sabab == DLL_PROCESS_ATTACH) {
        modul = h;
        DisableThreadLibraryCalls(h);
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;
    if (!IsEqualCLSID(rclsid, CLSID_OZBEK)) return CLASS_E_CLASSNOTAVAILABLE;
    return fabrika.QueryInterface(riid, ppv);
}

STDAPI DllCanUnloadNow() { return havolalar == 0 ? S_OK : S_FALSE; }

STDAPI DllRegisterServer() {
    HRESULT hr = com_yoz();
    if (SUCCEEDED(hr)) hr = profillar_yoz(true);
    if (SUCCEEDED(hr)) hr = toifalar_yoz(true);
    if (FAILED(hr)) DllUnregisterServer();
    return hr;
}

STDAPI DllUnregisterServer() {
    profillar_yoz(false);
    toifalar_yoz(false);
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, (L"Software\\Classes\\CLSID\\" + guid_matn(CLSID_OZBEK)).c_str());
    // UnregisterCategory faqat barglarni o'chiradi — TIP kaliti bo'sh ota kalitlari bilan qolardi
    for (REGSAM korinish : {KEY_WOW64_64KEY, KEY_WOW64_32KEY}) {
        HKEY k;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\CTF\\TIP", 0, KEY_ALL_ACCESS | korinish, &k) ==
            ERROR_SUCCESS) {
            RegDeleteTreeW(k, guid_matn(CLSID_OZBEK).c_str());
            RegCloseKey(k);
        }
    }
    return S_OK;
}
