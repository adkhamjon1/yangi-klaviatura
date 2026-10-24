// TSF matn xizmati: tugmani Dvigatel'ga beradi va javobni hujjatda bajaradi.
//   «darhol» — kursordan oldingi matn tekshirilib almashtiriladi (TSF'ni to'liq qo'llaydigan dasturlar);
//   «tagiga» — qaror kutayotgan harf kompozitsiyada turadi (eski/IMM32 dasturlar, TF_SS_TRANSITORY).
// Har TSF chaqiruvi istisnolardan himoyalangan: xizmat ichidagi xato ilovani yiqitmasligi kerak.
#include <functional>
#include <string>

#include "jadval.inc"
#include "panel.hpp"
#include "umumiy.hpp"

namespace xizmat {

using ozbek::Javob;
using ozbek::Matn;
using ozbek::Maxsus;
using ozbek::Rejim;
using ozbek::Tugma;

namespace {

// Tahrir sessiyasi: TSF hujjatni faqat shu ichida o'zgartirishga ruxsat beradi
class Tahrir final : public ITfEditSession {
public:
    explicit Tahrir(std::function<HRESULT(TfEditCookie)> f) : f_(std::move(f)) {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_ITfEditSession) {
            *ppv = static_cast<ITfEditSession*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&ref_); }
    STDMETHODIMP_(ULONG) Release() override {
        LONG r = InterlockedDecrement(&ref_);
        if (!r) delete this;
        return r;
    }
    STDMETHODIMP DoEditSession(TfEditCookie ec) override {
        try {
            return f_(ec);
        } catch (...) {
            return E_FAIL;
        }
    }

private:
    std::function<HRESULT(TfEditCookie)> f_;
    LONG ref_ = 1;
};

constexpr TfClientId MIJOZ_YOQ = 0;  // TF_CLIENTID_NULL (MinGW sarlavhasida yo'q)
// Xizmat ilovaga qayta yuborgan tugma: lParam'ning bo'sh (25–28) bitlaridan biri belgi bo'ladi
constexpr LPARAM QAYTA = 0x10000000;
constexpr UINT POCHTA_TUGMA = WM_APP + 0x4F5A, POCHTA_TIZIM = POCHTA_TUGMA + 1;
constexpr wchar_t POCHTA_SINFI[] = L"YangiKlaviaturaPochta";

// Qayta yuboriladigan tugma avval shu yashirin oynaga keladi, keyin fokusdagi oynaga jo'natiladi:
// IMM32 kompozitsiya natijasini ham posted xabar bilan beradi — tugma undan keyin kelishi kerak.
LRESULT CALLBACK pochta_jarayoni(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == POCHTA_TUGMA || m == POCHTA_TIZIM) {
        if (HWND f = GetFocus()) PostMessageW(f, m == POCHTA_TIZIM ? WM_SYSKEYDOWN : WM_KEYDOWN, w, l);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

// Sinf tizim jarayoni bilan ro'yxatga olinadi, o'zimizniki oynaga keyin ulanadi:
// DLL qayta yuklansa sinf eskirgan manzilga ishora qilmaydi
HWND pochta_yarat() {
    WNDCLASSEXW s = {};
    s.cbSize = sizeof s;
    if (!GetClassInfoExW(modul, POCHTA_SINFI, &s)) {
        s.lpfnWndProc = DefWindowProcW;
        s.hInstance = modul;
        s.lpszClassName = POCHTA_SINFI;
        RegisterClassExW(&s);
    }
    HWND h = CreateWindowExW(0, POCHTA_SINFI, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, modul, nullptr);
    if (h) SetWindowLongPtrW(h, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(pochta_jarayoni));
    return h;
}

template <class T>
void ozod(T*& p) {
    if (p) {
        p->Release();
        p = nullptr;
    }
}

std::wstring keng(const Matn& m) {
    std::u16string u = ozbek::utf16_ga(m);
    return std::wstring(u.begin(), u.end());
}

// Kursorni r oxiriga qo'yadi
void kursor_oxirga(TfEditCookie ec, ITfContext* ctx, ITfRange* r) {
    ITfRange* nusxa = nullptr;
    if (FAILED(r->Clone(&nusxa))) return;
    nusxa->Collapse(ec, TF_ANCHOR_END);
    TF_SELECTION tanlov = {};
    tanlov.range = nusxa;
    tanlov.style.ase = TF_AE_NONE;
    tanlov.style.fInterimChar = FALSE;
    ctx->SetSelection(ec, 1, &tanlov);
    nusxa->Release();
}

bool harf_mi(char32_t c) { return ozbek::harfmi(c); }

// O'ng Alt + W/C/[/] — asl belgi. Alt bilan bosilgan tugma OnKeyDown'ga kelmaydi (WM_SYSKEYDOWN),
// shuning uchun TSF «saqlangan tugma» (preserved key) sifatida ro'yxatdan o'tkaziladi.
struct AslTugma {
    UINT vk;
    bool shift;
    char32_t belgi;
    GUID guid;
};
constexpr GUID asl_guid(unsigned char n) {
    return {0x6c3d81a2, 0x7b44, 0x4f0e, {0x9a, 0x1d, 0x52, 0x8e, 0x0b, 0x7f, 0x31, n}};
}
const AslTugma ASL_TUGMALAR[] = {
    {'W', false, U'w', asl_guid(0)}, {'W', true, U'W', asl_guid(1)}, {'C', false, U'c', asl_guid(2)},
    {'C', true, U'C', asl_guid(3)},  {VK_OEM_4, false, U'[', asl_guid(4)}, {VK_OEM_4, true, U'{', asl_guid(5)},
    {VK_OEM_6, false, U']', asl_guid(6)}, {VK_OEM_6, true, U'}', asl_guid(7)},
};

TF_PRESERVEDKEY saqlangan(const AslTugma& a) {
    TF_PRESERVEDKEY k;
    k.uVKey = a.vk;
    k.uModifiers = TF_MOD_RALT | (a.shift ? TF_MOD_SHIFT : 0);
    return k;
}

}  // namespace

class Xizmat final : public ITfTextInputProcessor,
                     public ITfThreadMgrEventSink,
                     public ITfKeyEventSink,
                     public ITfCompositionSink,
                     public ITfInputProcessorProfileActivationSink,
                     public PanelEgasi {
public:
    Xizmat() { qosh(); }
    ~Xizmat() { ayir(); }

    // -- IUnknown --
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_INVALIDARG;
        if (riid == IID_IUnknown || riid == IID_ITfTextInputProcessor)
            *ppv = static_cast<ITfTextInputProcessor*>(this);
        else if (riid == IID_ITfThreadMgrEventSink)
            *ppv = static_cast<ITfThreadMgrEventSink*>(this);
        else if (riid == IID_ITfKeyEventSink)
            *ppv = static_cast<ITfKeyEventSink*>(this);
        else if (riid == IID_ITfCompositionSink)
            *ppv = static_cast<ITfCompositionSink*>(this);
        else if (riid == IID_ITfInputProcessorProfileActivationSink)
            *ppv = static_cast<ITfInputProcessorProfileActivationSink*>(this);
        else {
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&ref_); }
    STDMETHODIMP_(ULONG) Release() override {
        LONG r = InterlockedDecrement(&ref_);
        if (!r) delete this;
        return r;
    }

    // -- ITfTextInputProcessor --
    STDMETHODIMP Activate(ITfThreadMgr* tm, TfClientId id) override {
        try {
            tm_ = tm;
            tm_->AddRef();
            id_ = id;
            ITfSource* manba = nullptr;
            if (SUCCEEDED(tm_->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&manba)))) {
                manba->AdviseSink(IID_ITfThreadMgrEventSink, static_cast<ITfThreadMgrEventSink*>(this), &tm_kuki_);
                manba->AdviseSink(IID_ITfInputProcessorProfileActivationSink,
                                  static_cast<ITfInputProcessorProfileActivationSink*>(this), &profil_kuki_);
                manba->Release();
            }
            ITfKeystrokeMgr* km = nullptr;
            if (SUCCEEDED(tm_->QueryInterface(IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&km)))) {
                km->AdviseKeyEventSink(id_, static_cast<ITfKeyEventSink*>(this), TRUE);
                for (const auto& a : ASL_TUGMALAR) {
                    TF_PRESERVEDKEY k = saqlangan(a);
                    km->PreserveKey(id_, a.guid, &k, L"", 0);
                }
                km->Release();
            }
            CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                             IID_ITfInputProcessorProfileMgr, reinterpret_cast<void**>(&profil_));
            hkl_us_ = LoadKeyboardLayoutW(L"00000409", KLF_NOTELLSHELL);
            // o'rinbosar sxema (04090443) o'rnatilgandan keyin birinchi seansda hali yuklanmagan bo'ladi:
            // Windows uni kirishda yuklaydi, shu paytgacha xizmat o'zi yuklaydi
            LoadKeyboardLayoutW(L"00000443", KLF_NOTELLSHELL);
            sozla();
            pochta_ = pochta_yarat();
            if (s_.jurnal) jurnal_yoz(L"Activate " + std::to_wstring(reinterpret_cast<ULONG_PTR>(this)) + L" tm=" + std::to_wstring(reinterpret_cast<ULONG_PTR>(tm)));
            panel_ = new PanelTugma(this);
            ITfLangBarItemMgr* lm = nullptr;
            if (SUCCEEDED(tm_->QueryInterface(IID_ITfLangBarItemMgr, reinterpret_cast<void**>(&lm)))) {
                lm->AddItem(static_cast<ITfLangBarItem*>(panel_));
                lm->Release();
            }
            return S_OK;
        } catch (...) {
            Deactivate();
            return E_FAIL;
        }
    }
    STDMETHODIMP Deactivate() override {
        if (s_.jurnal) jurnal_yoz(L"Deactivate " + std::to_wstring(reinterpret_cast<ULONG_PTR>(this)));
        if (tm_) {
            if (panel_) {
                ITfLangBarItemMgr* lm = nullptr;
                if (SUCCEEDED(tm_->QueryInterface(IID_ITfLangBarItemMgr, reinterpret_cast<void**>(&lm)))) {
                    lm->RemoveItem(static_cast<ITfLangBarItem*>(panel_));
                    lm->Release();
                }
            }
            ITfKeystrokeMgr* km = nullptr;
            if (SUCCEEDED(tm_->QueryInterface(IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&km)))) {
                for (const auto& a : ASL_TUGMALAR) {
                    TF_PRESERVEDKEY k = saqlangan(a);
                    km->UnpreserveKey(a.guid, &k);
                }
                km->UnadviseKeyEventSink(id_);
                km->Release();
            }
            ITfSource* manba = nullptr;
            if (SUCCEEDED(tm_->QueryInterface(IID_ITfSource, reinterpret_cast<void**>(&manba)))) {
                if (tm_kuki_ != TF_INVALID_COOKIE) manba->UnadviseSink(tm_kuki_);
                if (profil_kuki_ != TF_INVALID_COOKIE) manba->UnadviseSink(profil_kuki_);
                manba->Release();
            }
        }
        if (panel_) {
            panel_->uzil();
            panel_->Release();
            panel_ = nullptr;
        }
        if (pochta_) {
            DestroyWindow(pochta_);
            pochta_ = nullptr;
        }
        tm_kuki_ = profil_kuki_ = TF_INVALID_COOKIE;
        ozod(komp_);
        ozod(profil_);
        ozod(tm_);
        id_ = MIJOZ_YOQ;
        return S_OK;
    }

    // -- ITfThreadMgrEventSink --
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    STDMETHODIMP OnSetFocus(ITfDocumentMgr*, ITfDocumentMgr*) override {
        dv_.unut();
        ozod(komp_);
        darholsiz_ = nullptr;
        return S_OK;
    }
    STDMETHODIMP OnPushContext(ITfContext*) override { return S_OK; }
    STDMETHODIMP OnPopContext(ITfContext*) override { return S_OK; }

    // -- ITfKeyEventSink --
    STDMETHODIMP OnSetFocus(BOOL) override { return S_OK; }
    STDMETHODIMP OnTestKeyDown(ITfContext* ctx, WPARAM wp, LPARAM lp, BOOL* yedi) override {
        if (!yedi) return E_INVALIDARG;
        *yedi = FALSE;
        if (lp & QAYTA) return S_OK;
        if (!ctx) {
            if (s_.jurnal) jurnal_yoz(L"test ctx=null vk=" + std::to_wstring(wp));
            return S_OK;
        }
        try {
            *yedi = kerakmi(ctx, wp);
        } catch (...) {
            *yedi = FALSE;
        }
        return S_OK;
    }
    STDMETHODIMP OnKeyDown(ITfContext* ctx, WPARAM wp, LPARAM lp, BOOL* yedi) override {
        if (!yedi) return E_INVALIDARG;
        *yedi = FALSE;
        if (!ctx || (lp & QAYTA)) return S_OK;
        try {
            if (kerakmi(ctx, wp)) *yedi = ishla(ctx, wp, lp);
        } catch (...) {
            dv_.unut();
            *yedi = FALSE;
        }
        return S_OK;
    }
    STDMETHODIMP OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* yedi) override {
        if (yedi) *yedi = FALSE;
        return S_OK;
    }
    STDMETHODIMP OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* yedi) override {
        if (yedi) *yedi = FALSE;
        return S_OK;
    }
    STDMETHODIMP OnPreservedKey(ITfContext* ctx, REFGUID g, BOOL* yedi) override {
        if (!yedi) return E_INVALIDARG;
        *yedi = FALSE;
        if (!ctx) return S_OK;
        try {
            for (const auto& a : ASL_TUGMALAR) {
                if (!IsEqualGUID(g, a.guid)) continue;
                if (kuzat_.ozgardimi()) sozla();
                const Rejim r = joriy_rejim();
                if (r != dv_.rejim) {
                    dv_.unut();
                    dv_.rejim = r;
                }
                // faqat «öz» da tugmalar yangi harf berganda: aks holda o'ng Alt ilovaning o'ziniki
                if (r != Rejim::Oz || !s_.dvigatel.tugmalar) return S_OK;
                Tugma t;
                t.belgi = a.belgi;
                t.altgr = true;
                *yedi = bajar(ctx, t, a.vk, 0);
                if (s_.jurnal) jurnal_yoz(L"saqlangan " + keng(Matn(1, a.belgi)) + L" → " + std::to_wstring(*yedi));
                break;
            }
        } catch (...) {
            dv_.unut();
            *yedi = FALSE;
        }
        return S_OK;
    }

    // -- ITfCompositionSink: ilova kompozitsiyani o'zi tugatdi (masalan, sichqoncha bilan bosildi) --
    STDMETHODIMP OnCompositionTerminated(TfEditCookie, ITfComposition* komp) override {
        if (komp_ && komp == komp_) ozod(komp_);
        dv_.kompozitsiya_tugadi();
        return S_OK;
    }

    // -- ITfInputProcessorProfileActivationSink: öz ↔ o'z ↔ ўз almashganda xizmat qayta yuklanmaydi --
    STDMETHODIMP OnActivated(DWORD, LANGID, REFCLSID clsid, REFGUID, REFGUID, HKL, DWORD bayroq) override {
        if (s_.jurnal && IsEqualCLSID(clsid, CLSID_OZBEK)) jurnal_yoz(L"OnActivated bayroq=" + std::to_wstring(bayroq));
        if ((bayroq & TF_IPSINK_FLAG_ACTIVE) && IsEqualCLSID(clsid, CLSID_OZBEK) && panel_) panel_->yangila();
        return S_OK;
    }

    // -- PanelEgasi --
    ozbek::Rejim panel_rejimi() override { return joriy_rejim(); }
    void panel_sozladi() override {
        dv_.unut();
        sozla();
    }

private:
    LONG ref_ = 1;
    ITfThreadMgr* tm_ = nullptr;
    TfClientId id_ = MIJOZ_YOQ;
    DWORD tm_kuki_ = TF_INVALID_COOKIE;
    DWORD profil_kuki_ = TF_INVALID_COOKIE;
    PanelTugma* panel_ = nullptr;
    HWND pochta_ = nullptr;
    ITfInputProcessorProfileMgr* profil_ = nullptr;
    ITfComposition* komp_ = nullptr;
    HKL hkl_us_ = nullptr;
    ITfContext* darholsiz_ = nullptr;  // atrofdagi matnni o'qib bo'lmagan hujjat (faqat solishtirish uchun)

    ozbek::Dvigatel dv_;
    ozbek::Lugat lugat_;
    WinSozlama s_;
    Kuzatuvchi kuzat_;

    void sozla() {
        s_ = sozlama_oqi();
        lugat_ = lugat_yasa();
        dv_.s = s_.dvigatel;
        dv_.lugat = &lugat_;
        dv_.sozla();
    }

    Rejim joriy_rejim() {
        TF_INPUTPROCESSORPROFILE p = {};
        if (profil_ && SUCCEEDED(profil_->GetActiveProfile(GUID_TFCAT_TIP_KEYBOARD, &p)) &&
            IsEqualCLSID(p.clsid, CLSID_OZBEK)) {
            if (IsEqualGUID(p.guidProfile, PROFIL_ESKI)) return Rejim::Eski;
            if (IsEqualGUID(p.guidProfile, PROFIL_KIRILL)) return Rejim::Kirill;
        }
        return Rejim::Oz;
    }

    static bool bosilgan(int vk) { return (GetKeyState(vk) & 0x8000) != 0; }
    // O'ng Alt: profil ostidagi US sxemasida u Ctrl yubormaydi (AltGr'li sxemalarda Ctrl+Alt bo'lib keladi)
    static bool altgr_mi() { return bosilgan(VK_RMENU) && !bosilgan(VK_LMENU); }

    // TSF bu tugmani bizga beradimi (OnTestKeyDown va OnKeyDown bir xil javob berishi kerak)
    BOOL kerakmi(ITfContext* ctx, WPARAM wp) {
        BOOL k = kerakmi_(ctx, wp);
        if (s_.jurnal)
            jurnal_yoz(L"kerakmi vk=" + std::to_wstring(wp) + L" rejim=" + std::to_wstring(static_cast<int>(dv_.rejim)) +
                       L" ctrl=" + std::to_wstring(bosilgan(VK_CONTROL)) + L" alt=" + std::to_wstring(bosilgan(VK_MENU)) +
                       L" ralt=" + std::to_wstring(bosilgan(VK_RMENU)) + L" → " + std::to_wstring(k));
        return k;
    }

    BOOL kerakmi_(ITfContext*, WPARAM wp) {
        if (kuzat_.ozgardimi()) sozla();
        Rejim r = joriy_rejim();
        if (r != dv_.rejim) {
            dv_.unut();
            dv_.rejim = r;
        }
        const bool kutish = !dv_.ogirgich().kutish.empty() || komp_;
        const bool ctrl = bosilgan(VK_CONTROL), alt = bosilgan(VK_MENU);
        const bool altgr = altgr_mi();
        const bool win = bosilgan(VK_LWIN) || bosilgan(VK_RWIN);
        if ((ctrl || alt || win) && !altgr) return kutish;  // faqat kutayotgan harfni qoldirish uchun
        if (r == Rejim::Eski && !s_.dvigatel.eski_belgi) return FALSE;
        switch (wp) {
            case VK_BACK: case VK_RETURN: case VK_TAB: case VK_SPACE:
                return r != Rejim::Eski;
            case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN: case VK_HOME: case VK_END:
            case VK_PRIOR: case VK_NEXT: case VK_DELETE: case VK_ESCAPE: case VK_INSERT:
                return kutish;
            default:
                break;
        }
        return belgi_ber(wp, 0, altgr) != 0 || kutish;
    }

    // US sxemasi bo'yicha belgi (AltGr bo'lsa asl harf: AltGr+W → w)
    char32_t belgi_ber(WPARAM wp, UINT skankod, bool altgr) {
        BYTE holat[256];
        if (!GetKeyboardState(holat)) return 0;
        if (altgr) holat[VK_CONTROL] = holat[VK_LCONTROL] = holat[VK_RCONTROL] = holat[VK_MENU] = holat[VK_LMENU] =
            holat[VK_RMENU] = 0;
        if (!skankod) skankod = MapVirtualKeyExW(static_cast<UINT>(wp), MAPVK_VK_TO_VSC, hkl_us_);
        WCHAR buf[8];
        int n = ToUnicodeEx(static_cast<UINT>(wp), skankod, holat, buf, 8, 0x4 /* holatni o'zgartirmaslik */, hkl_us_);
        return (n == 1 && buf[0] >= 0x20) ? static_cast<char32_t>(buf[0]) : 0;
    }

    Tugma tugmaga(WPARAM wp, LPARAM lp) {
        Tugma t;
        const bool ctrl = bosilgan(VK_CONTROL), alt = bosilgan(VK_MENU);
        t.altgr = altgr_mi();
        t.boshqaruv = (ctrl || alt || bosilgan(VK_LWIN) || bosilgan(VK_RWIN)) && !t.altgr;
        switch (wp) {
            case VK_BACK: t.maxsus = Maxsus::Backspace; return t;
            case VK_RETURN: t.maxsus = Maxsus::Enter; return t;
            case VK_TAB: t.maxsus = Maxsus::Tab; return t;
            case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN: case VK_HOME: case VK_END:
            case VK_PRIOR: case VK_NEXT: case VK_DELETE: case VK_ESCAPE: case VK_INSERT:
                t.maxsus = Maxsus::Kursor;
                return t;
            default:
                break;
        }
        const UINT skankod = (static_cast<UINT>(lp) >> 16) & 0xFF;
        const char32_t us = belgi_ber(wp, skankod, t.altgr);
        t.ilova = t.altgr ? 0 : us;  // AltGr+W ni ilova o'zi yozmaydi
        t.belgi = us;
        if (dv_.rejim == Rejim::Kirill && !t.altgr) {
            for (const auto& k : KIRILL_TUGMALAR) {
                if (k.skankod != skankod) continue;
                const bool shift = bosilgan(VK_SHIFT), caps = (GetKeyState(VK_CAPITAL) & 1) != 0;
                const bool katta = harf_mi(k.oddiy) ? (shift != caps) : shift;
                t.belgi = katta ? k.shift : k.oddiy;
                break;
            }
        }
        if (wp == VK_SPACE) t.belgi = t.ilova = U' ';
        if (!t.belgi && t.maxsus == Maxsus::Yoq) t.maxsus = Maxsus::Boshqa;
        return t;
    }

    // IMM32 orqali ishlaydigan oddiy dastur (Notepad, EDIT maydoni): CUAS ning vaqtinchalik hujjati
    static bool imm32_mi(ITfContext* ctx) {
        TF_STATUS holat = {};
        return SUCCEEDED(ctx->GetStatus(&holat)) && (holat.dwStaticFlags & TF_SS_TRANSITORY);
    }

    bool darhol_mi(ITfContext* ctx) {
        if (!s_.darhol || ctx == darholsiz_) return false;
        return !imm32_mi(ctx);
    }

    // IMM32'da OnTestKeyDown «yeyman» degan tugma ilovaga VK_PROCESSKEY bo'lib boradi: yeyilmasa yo'qoladi.
    // Shuning uchun u fokusdagi oynaga qayta yuboriladi — posted xabar navbatdagi tugmalardan oldin keladi.
    void qayta_yubor(WPARAM wp, LPARAM lp) {
        const bool tizim = (GetKeyState(VK_MENU) & 0x8000) && !(GetKeyState(VK_CONTROL) & 0x8000);
        if (pochta_) {
            PostMessageW(pochta_, tizim ? POCHTA_TIZIM : POCHTA_TUGMA, wp, lp | QAYTA);
        } else if (HWND f = GetFocus()) {
            PostMessageW(f, tizim ? WM_SYSKEYDOWN : WM_KEYDOWN, wp, lp | QAYTA);
        }
    }

    BOOL ishla(ITfContext* ctx, WPARAM wp, LPARAM lp) { return bajar(ctx, tugmaga(wp, lp), wp, lp); }

    BOOL bajar(ITfContext* ctx, const Tugma& t, WPARAM wp, LPARAM lp) {
        const bool darhol = darhol_mi(ctx);
        if (darhol && dv_.oldingi_kerak()) dv_.oldingi_berildi(oldingi_belgi(ctx));
        Javob j = dv_.tugma(t, darhol);
        if (!j.organ.empty()) {
            lugatga_qosh(j.organ);
            lugat_ = lugat_yasa();
            dv_.sozla();
        }
        bool yedi = j.yedi;
        if (s_.jurnal)
            jurnal_yoz(L"ishla belgi=" + keng(Matn(1, t.belgi)) + L" ilova=" + keng(Matn(1, t.ilova)) +
                       L" darhol=" + std::to_wstring(darhol) + L" yedi=" + std::to_wstring(j.yedi) + L" oldingi=«" +
                       keng(j.oldingi) + L"» yoz=«" + keng(j.yoz) + L"» tasdiq=«" + keng(j.tasdiq) + L"» komp=«" +
                       keng(j.kompozitsiya) + L"»");
        if (darhol) {
            if (j.ozgarish()) {
                Natija n = almashtir(ctx, j.oldingi, j.yoz);
                if (s_.jurnal) jurnal_yoz(L"almashtir → " + std::to_wstring(static_cast<int>(n)));
                if (n != Natija::Bajarildi) {
                    // kursor ko'chgan yoki dastur matnni bermaydi: tugma ilovaga o'tadi
                    dv_.unut();
                    if (n == Natija::Imkonsiz) darholsiz_ = ctx;
                    yedi = false;
                }
            }
        } else if (j.kompozitsiya_bor || !j.tasdiq.empty() || !j.yoz.empty()) {
            kompozitsiya(ctx, j.tasdiq + j.yoz, j.kompozitsiya);
        }
        if (!yedi && lp && imm32_mi(ctx)) {
            qayta_yubor(wp, lp);
            yedi = true;
        }
        return yedi ? TRUE : FALSE;
    }

    HRESULT sessiya(ITfContext* ctx, DWORD tur, std::function<HRESULT(TfEditCookie)> f) {
        Tahrir* t = new Tahrir(std::move(f));
        HRESULT natija = E_FAIL;
        HRESULT hr = ctx->RequestEditSession(id_, t, TF_ES_SYNC | tur, &natija);
        t->Release();
        return FAILED(hr) ? hr : natija;
    }

    char32_t oldingi_belgi(ITfContext* ctx) {
        char32_t c = 0;
        sessiya(ctx, TF_ES_READ, [&](TfEditCookie ec) {
            TF_SELECTION tanlov = {};
            ULONG soni = 0;
            if (FAILED(ctx->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tanlov, &soni)) || soni != 1) return E_FAIL;
            LONG kochdi = 0;
            WCHAR buf[2];
            ULONG olindi = 0;
            if (SUCCEEDED(tanlov.range->ShiftStart(ec, -1, &kochdi, nullptr)) && kochdi == -1 &&
                SUCCEEDED(tanlov.range->GetText(ec, 0, buf, 1, &olindi)) && olindi == 1)
                c = buf[0];
            tanlov.range->Release();
            return S_OK;
        });
        return c;
    }

    enum class Natija { Bajarildi, MosEmas, Imkonsiz };

    // Kursordan oldingi `oldingi` matnini `yoz` ga almashtiradi — avval rostdan shu matn ekanini tekshiradi
    Natija almashtir(ITfContext* ctx, const Matn& oldingi, const Matn& yoz) {
        const std::wstring eski = keng(oldingi), yangi = keng(yoz);
        Natija natija = Natija::Imkonsiz;
        sessiya(ctx, TF_ES_READWRITE, [&](TfEditCookie ec) {
            if (eski.empty()) {
                ITfInsertAtSelection* qoy = nullptr;
                if (FAILED(ctx->QueryInterface(IID_ITfInsertAtSelection, reinterpret_cast<void**>(&qoy)))) return E_FAIL;
                ITfRange* r = nullptr;
                HRESULT hr = qoy->InsertTextAtSelection(ec, 0, yangi.c_str(), static_cast<LONG>(yangi.size()), &r);
                qoy->Release();
                if (FAILED(hr) || !r) return E_FAIL;
                kursor_oxirga(ec, ctx, r);
                r->Release();
                natija = Natija::Bajarildi;
                return S_OK;
            }
            TF_SELECTION tanlov = {};
            ULONG soni = 0;
            if (FAILED(ctx->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tanlov, &soni)) || soni != 1) return E_FAIL;
            ITfRange* r = tanlov.range;
            BOOL bosh = FALSE;
            r->IsEmpty(ec, &bosh);
            LONG kochdi = 0;
            const LONG kerak = -static_cast<LONG>(eski.size());
            if (!bosh) {
                natija = Natija::MosEmas;  // foydalanuvchi matn belgilagan
            } else if (FAILED(r->ShiftStart(ec, kerak, &kochdi, nullptr)) || kochdi != kerak) {
                natija = kochdi == 0 ? Natija::Imkonsiz : Natija::MosEmas;
            } else {
                std::wstring bor(eski.size() + 1, L'\0');
                ULONG olindi = 0;
                if (FAILED(r->GetText(ec, 0, bor.data(), static_cast<ULONG>(eski.size()), &olindi))) {
                    natija = Natija::Imkonsiz;
                } else if (olindi != eski.size() || bor.compare(0, olindi, eski) != 0) {
                    natija = Natija::MosEmas;
                } else if (SUCCEEDED(r->SetText(ec, 0, yangi.c_str(), static_cast<LONG>(yangi.size())))) {
                    kursor_oxirga(ec, ctx, r);
                    natija = Natija::Bajarildi;
                }
            }
            r->Release();
            return S_OK;
        });
        return natija;
    }

    // «tagiga»: `tasdiq` hujjatga yoziladi, `korinish` kompozitsiyada qoladi
    void kompozitsiya(ITfContext* ctx, const Matn& tasdiq, const Matn& korinish) {
        const std::wstring t = keng(tasdiq), k = keng(korinish), hammasi = t + k;
        sessiya(ctx, TF_ES_READWRITE, [&](TfEditCookie ec) {
            if (komp_) {
                ITfRange* r = nullptr;
                if (FAILED(komp_->GetRange(&r)) || !r) return E_FAIL;
                r->SetText(ec, 0, hammasi.c_str(), static_cast<LONG>(hammasi.size()));
                kursor_oxirga(ec, ctx, r);
                if (k.empty()) {
                    komp_->EndComposition(ec);
                    ozod(komp_);
                } else if (!t.empty()) {
                    ITfRange* qolgan = nullptr;
                    LONG kochdi = 0;
                    if (SUCCEEDED(r->Clone(&qolgan))) {
                        qolgan->ShiftStart(ec, static_cast<LONG>(t.size()), &kochdi, nullptr);
                        komp_->ShiftStart(ec, qolgan);
                        qolgan->Release();
                    }
                }
                r->Release();
                return S_OK;
            }
            ITfInsertAtSelection* qoy = nullptr;
            if (FAILED(ctx->QueryInterface(IID_ITfInsertAtSelection, reinterpret_cast<void**>(&qoy)))) return E_FAIL;
            if (!t.empty()) {
                ITfRange* r = nullptr;
                if (SUCCEEDED(qoy->InsertTextAtSelection(ec, 0, t.c_str(), static_cast<LONG>(t.size()), &r)) && r) {
                    kursor_oxirga(ec, ctx, r);
                    r->Release();
                }
            }
            if (!k.empty()) {
                ITfRange* joy = nullptr;
                ITfContextComposition* cc = nullptr;
                if (SUCCEEDED(qoy->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, nullptr, 0, &joy)) && joy &&
                    SUCCEEDED(ctx->QueryInterface(IID_ITfContextComposition, reinterpret_cast<void**>(&cc)))) {
                    if (SUCCEEDED(cc->StartComposition(ec, joy, static_cast<ITfCompositionSink*>(this), &komp_)) && komp_) {
                        ITfRange* kr = nullptr;
                        if (SUCCEEDED(komp_->GetRange(&kr)) && kr) {
                            kr->SetText(ec, 0, k.c_str(), static_cast<LONG>(k.size()));
                            kursor_oxirga(ec, ctx, kr);
                            kr->Release();
                        }
                    }
                }
                ozod(cc);
                ozod(joy);
            }
            qoy->Release();
            return S_OK;
        });
    }
};

HRESULT xizmat_yarat(REFIID riid, void** ppv) {
    Xizmat* x = new (std::nothrow) Xizmat();
    if (!x) return E_OUTOFMEMORY;
    HRESULT hr = x->QueryInterface(riid, ppv);
    x->Release();
    return hr;
}

}  // namespace xizmat
