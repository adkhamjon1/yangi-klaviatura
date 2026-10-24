// Til panelidagi rejim tugmasi (panel.hpp).
#include "panel.hpp"

#include <olectl.h>
#include <shellapi.h>

#include <cwchar>
#include <utility>

#include "resurs.h"

namespace xizmat {

using ozbek::Rejim;

namespace {

constexpr DWORD KUKI = 1;
constexpr UINT SOZLAMALAR_BANDI = 100;
constexpr wchar_t MENYU_SINFI[] = L"YangiKlaviaturaMenyu";

struct Band {
    const wchar_t* kalit;  // HKCU\Software\YangiKlaviatura dagi DWORD
    const wchar_t* nom;
};
const Band OZ_BANDLARI[] = {{L"tugmalar", L"Yangi harflar tugmalarda"}, {L"lotin", L"Eski lotindan o'girish"}};
const Band ESKI_BANDLARI[] = {{L"eski_belgi", L"oʻ va gʻ belgisi"}};
const Band KIRILL_BANDLARI[] = {{L"kirill", L"Yangi lotinga o'girish"}};

std::pair<const Band*, UINT> bandlar(Rejim r) {
    switch (r) {
        case Rejim::Eski: return {ESKI_BANDLARI, 1};
        case Rejim::Kirill: return {KIRILL_BANDLARI, 1};
        default: return {OZ_BANDLARI, 2};
    }
}

const wchar_t* qisqa(Rejim r) { return r == Rejim::Eski ? L"O'Z" : r == Rejim::Kirill ? L"ЎЗ" : L"ÖZ"; }
const wchar_t* izohi(Rejim r) {
    return r == Rejim::Eski ? L"O'zbek — eski lotin (O'Z). Bosing: sozlamalar"
           : r == Rejim::Kirill ? L"Ўзбек — kirilldan yangi lotinga (ЎЗ). Bosing: sozlamalar"
                                : L"Özbek — yangi yozuv (ÖZ). Bosing: sozlamalar";
}

// Menyu egasi bo'ladigan ko'rinmas oyna. Oyna jarayoni tizimniki (DefWindowProcW):
// DLL bo'shatilib qayta yuklansa ham ro'yxatdagi sinf eskirgan manzilga ishora qilmaydi.
HWND menyu_oynasi() {
    WNDCLASSEXW s = {};
    s.cbSize = sizeof s;
    if (!GetClassInfoExW(modul, MENYU_SINFI, &s)) {
        s.lpfnWndProc = DefWindowProcW;
        s.hInstance = modul;
        s.lpszClassName = MENYU_SINFI;
        RegisterClassExW(&s);
    }
    return CreateWindowExW(WS_EX_TOOLWINDOW, MENYU_SINFI, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, modul, nullptr);
}

}  // namespace

PanelTugma::PanelTugma(PanelEgasi* egasi) : egasi_(egasi) { qosh(); }

PanelTugma::~PanelTugma() {
    if (sink_) sink_->Release();
    if (oyna_) DestroyWindow(oyna_);
    ayir();
}

STDMETHODIMP PanelTugma::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_INVALIDARG;
    if (riid == IID_IUnknown || riid == IID_ITfLangBarItem || riid == IID_ITfLangBarItemButton_)
        *ppv = static_cast<ITfLangBarItemButton_*>(this);
    else if (riid == IID_ITfSource)
        *ppv = static_cast<ITfSource*>(this);
    else {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) PanelTugma::Release() {
    LONG r = InterlockedDecrement(&ref_);
    if (!r) delete this;
    return r;
}

Rejim PanelTugma::rejim() const { return egasi_ ? egasi_->panel_rejimi() : Rejim::Oz; }

void PanelTugma::yangila() {
    if (sink_) sink_->OnUpdate(TF_LBI_ICON | TF_LBI_TEXT | TF_LBI_TOOLTIP);
}

STDMETHODIMP PanelTugma::GetInfo(TF_LANGBARITEMINFO* info) {
    if (!info) return E_INVALIDARG;
    *info = {};
    info->clsidService = CLSID_OZBEK;
    info->guidItem = GUID_LBI_INPUTMODE_;
    info->dwStyle = TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_SHOWNINTRAY;
    info->ulSort = 0;
    wcsncpy(info->szDescription, NOM, TF_LBI_DESC_MAXLEN - 1);
    return S_OK;
}

STDMETHODIMP PanelTugma::GetStatus(DWORD* holat) {
    if (!holat) return E_INVALIDARG;
    *holat = 0;
    return S_OK;
}

STDMETHODIMP PanelTugma::GetTooltipString(BSTR* izoh) {
    if (!izoh) return E_INVALIDARG;
    *izoh = SysAllocString(izohi(rejim()));
    return *izoh ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP PanelTugma::GetText(BSTR* matn) {
    if (!matn) return E_INVALIDARG;
    *matn = SysAllocString(qisqa(rejim()));
    return *matn ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP PanelTugma::GetIcon(HICON* ikon) {
    if (!ikon) return E_INVALIDARG;
    // Rejim (ÖZ, O'Z, ЎЗ) til belgisida turibdi; bu tugmada dastur belgisi — tray'da ikki bir xil belgi
    // bo'lmasin. TSF ikonkani o'zi yo'q qiladi — har safar yangisi beriladi.
    *ikon = static_cast<HICON>(LoadImageW(modul, MAKEINTRESOURCEW(IKONKA_ILOVA), IMAGE_ICON,
                                          GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
    return *ikon ? S_OK : E_FAIL;
}

// Klassik til paneli menyuni o'zi chizadi: bandlar shu yerda beriladi
STDMETHODIMP PanelTugma::InitMenu(ITfMenu_* menyu) {
    if (!menyu) return E_INVALIDARG;
    try {
        auto [b, soni] = bandlar(rejim());
        for (UINT i = 0; i < soni; ++i)
            menyu->AddMenuItem(i + 1, sozlama_ber(b[i].kalit) ? TF_LBMENUF_CHECKED : 0, nullptr, nullptr, b[i].nom,
                               static_cast<ULONG>(wcslen(b[i].nom)), nullptr);
        menyu->AddMenuItem(0, TF_LBMENUF_SEPARATOR, nullptr, nullptr, nullptr, 0, nullptr);
        menyu->AddMenuItem(SOZLAMALAR_BANDI, 0, nullptr, nullptr, L"Sozlamalar…", 11, nullptr);
    } catch (...) {
    }
    return S_OK;
}

STDMETHODIMP PanelTugma::OnMenuSelect(UINT id) {
    try {
        tanla(id);
    } catch (...) {
    }
    return S_OK;
}

// Tray'dagi belgi (Windows 8+) menyu so'ramaydi, faqat bosilganini aytadi: menyu shu yerda ko'rsatiladi
STDMETHODIMP PanelTugma::OnClick(TfLBIClick, POINT nuqta, const RECT* joy) {
    try {
        menyu_korsat(nuqta, joy);
    } catch (...) {
    }
    return S_OK;
}

void PanelTugma::menyu_korsat(POINT nuqta, const RECT* joy) {
    if (!oyna_) oyna_ = menyu_oynasi();
    HMENU m = CreatePopupMenu();
    if (!oyna_ || !m) {
        if (m) DestroyMenu(m);
        return;
    }
    auto [b, soni] = bandlar(rejim());
    for (UINT i = 0; i < soni; ++i)
        AppendMenuW(m, MF_STRING | (sozlama_ber(b[i].kalit) ? MF_CHECKED : MF_UNCHECKED), i + 1, b[i].nom);
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING, SOZLAMALAR_BANDI, L"Sozlamalar…");

    // Menyu tashqariga bosilganda yopilishi uchun egasi oldingi oynaga chiqariladi, keyin fokus qaytariladi
    HWND oldingi = GetForegroundWindow();
    SetForegroundWindow(oyna_);
    TPMPARAMS tp = {};
    tp.cbSize = sizeof tp;
    if (joy) tp.rcExclude = *joy;
    UINT id = static_cast<UINT>(TrackPopupMenuEx(m, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_VERTICAL,
                                                 nuqta.x, nuqta.y, oyna_, joy ? &tp : nullptr));
    PostMessageW(oyna_, WM_NULL, 0, 0);
    DestroyMenu(m);
    if (oldingi && oldingi != oyna_ && GetForegroundWindow() == oyna_) SetForegroundWindow(oldingi);
    if (id) tanla(id);
}

void PanelTugma::tanla(UINT id) {
    if (id == SOZLAMALAR_BANDI) {
        const std::wstring yol = modul_papkasi() + L"\\Sozlamalar.exe";
        ShellExecuteW(nullptr, L"open", yol.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }
    auto [b, soni] = bandlar(rejim());
    if (id < 1 || id > soni) return;
    sozlama_yoz(b[id - 1].kalit, !sozlama_ber(b[id - 1].kalit));
    if (egasi_) egasi_->panel_sozladi();
    yangila();
}

STDMETHODIMP PanelTugma::AdviseSink(REFIID riid, IUnknown* sink, DWORD* kuki) {
    if (!sink || !kuki) return E_INVALIDARG;
    if (riid != IID_ITfLangBarItemSink) return CONNECT_E_CANNOTCONNECT;
    if (sink_) return CONNECT_E_ADVISELIMIT;
    if (FAILED(sink->QueryInterface(IID_ITfLangBarItemSink, reinterpret_cast<void**>(&sink_)))) {
        sink_ = nullptr;
        return E_NOINTERFACE;
    }
    *kuki = KUKI;
    return S_OK;
}

STDMETHODIMP PanelTugma::UnadviseSink(DWORD kuki) {
    if (kuki != KUKI || !sink_) return CONNECT_E_NOCONNECTION;
    sink_->Release();
    sink_ = nullptr;
    return S_OK;
}

}  // namespace xizmat
