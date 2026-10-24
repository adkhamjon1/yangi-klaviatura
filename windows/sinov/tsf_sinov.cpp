// Haqiqiy Windows'dagi sinov: o'rnatilgan matn xizmati odam yozgandek tekshiriladi.
//   tsf_sinov.exe  →  profillar, yozish (EDIT va TSF'li RichEdit, SendInput), panel tugmasi va menyusi
// VM ichida windows/vm/vm.py yugur bilan yuradi. Chiqish UTF-8; oxirgi qator «NATIJA o'tdi/jami».
// Sinov HKCU\Software\YangiKlaviatura va foydalanuvchi lug'atini o'zgartiradi, oxirida o'chiradi.
#include <windows.h>
#include <msctf.h>
#include <richedit.h>
#include <tlhelp32.h>

#include <cstdio>
#include <string>
#include <vector>

#include "../tsf/jadval.inc"
#include "../tsf/panel.hpp"

using namespace xizmat;

// MinGW msctf.h da yo'q (Windows SDK qiymatlari)
#ifndef TF_IPPMF_FORPROCESS
#define TF_IPPMF_FORPROCESS 0x10000000
#define TF_IPPMF_DONTCARECURRENTINPUTLANGUAGE 0x00000004
#endif
#ifndef TF_IPP_FLAG_ENABLED
#define TF_IPP_FLAG_ENABLED 0x00000002
#endif

namespace {

int otdi = 0, jami = 0;
HKL us = nullptr;
HWND asosiy = nullptr;
ITfInputProcessorProfileMgr* pm = nullptr;

std::string utf8(const std::wstring& s) {
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    std::string u(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), u.data(), n, nullptr, nullptr);
    return u;
}

void chiqar(const std::wstring& s) {
    std::string u = utf8(s) + "\n";
    fwrite(u.data(), 1, u.size(), stdout);
    fflush(stdout);
}

void tekshir(bool ok, const std::wstring& nom, const std::wstring& tafsilot = L"") {
    ++jami;
    if (ok) ++otdi;
    chiqar((ok ? L"✓ " : L"✗ ") + nom + (ok || tafsilot.empty() ? L"" : L"  — " + tafsilot));
}

void solishtir(const std::wstring& nom, const std::wstring& kutilgan, const std::wstring& bor) {
    tekshir(kutilgan == bor, nom, L"kutilgan «" + kutilgan + L"», chiqdi «" + bor + L"»");
}

// Xabarlarni ms davomida qayta ishlaydi (SendInput navbatga qo'yadi, TSF shu yerda ishlaydi)
void pompa(int ms) {
    const DWORD oxir = GetTickCount() + ms;
    MSG m;
    while (static_cast<int>(oxir - GetTickCount()) > 0) {
        while (PeekMessageW(&m, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
        MsgWaitForMultipleObjects(0, nullptr, FALSE, 10, QS_ALLINPUT);
    }
}

void kirit(WORD vk, WORD skan, DWORD bayroq) {
    INPUT i = {};
    i.type = INPUT_KEYBOARD;
    i.ki.wVk = vk;
    i.ki.wScan = skan;
    i.ki.dwFlags = bayroq;
    SendInput(1, &i, sizeof i);
}

void tugma(UINT vk, bool shift = false, bool ong_alt = false, UINT skan = 0) {
    if (!skan) skan = MapVirtualKeyExW(vk, MAPVK_VK_TO_VSC, us);
    if (shift) kirit(VK_LSHIFT, 0x2A, 0);
    if (ong_alt) kirit(VK_RMENU, 0x38, KEYEVENTF_EXTENDEDKEY);
    kirit(static_cast<WORD>(vk), static_cast<WORD>(skan), 0);
    kirit(static_cast<WORD>(vk), static_cast<WORD>(skan), KEYEVENTF_KEYUP);
    if (ong_alt) kirit(VK_RMENU, 0x38, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP);
    if (shift) kirit(VK_LSHIFT, 0x2A, KEYEVENTF_KEYUP);
    pompa(30);
}

// US tugmalari bilan (lotin rejimlari)
void yoz(const std::wstring& s) {
    for (wchar_t c : s) {
        if (c == L'\b') {
            tugma(VK_BACK);
            continue;
        }
        SHORT k = VkKeyScanExW(c, us);
        tugma(LOBYTE(k), HIBYTE(k) & 1);
    }
    pompa(150);
}

// O'zbek kirill tugmalari bilan (skankodlar jadvaldan)
void yoz_kirill(const std::wstring& s) {
    for (wchar_t c : s) {
        if (c == L' ') {
            tugma(VK_SPACE);
            continue;
        }
        for (const auto& k : KIRILL_TUGMALAR) {
            if (static_cast<wchar_t>(k.oddiy) != c && static_cast<wchar_t>(k.shift) != c) continue;
            UINT vk = MapVirtualKeyExW(k.skankod, MAPVK_VSC_TO_VK, us);
            tugma(vk, static_cast<wchar_t>(k.oddiy) != c, false, k.skankod);
            break;
        }
    }
    pompa(150);
}

std::wstring matn(HWND h) {
    int n = GetWindowTextLengthW(h);
    std::wstring s(n + 1, L'\0');
    GetWindowTextW(h, s.data(), n + 1);
    s.resize(n);
    return s;
}

void tozala(HWND h) {
    SetWindowTextW(h, L"");
    SetFocus(h);
    pompa(150);
}

// Fonda ishga tushgan jarayon oynasini Windows oldinga chiqarmaydi — odamdek sarlavhaga bosiladi
void sarlavhaga_bos() {
    RECT r;
    GetWindowRect(asosiy, &r);
    const int x = (r.left + r.right) / 2, y = r.top + GetSystemMetrics(SM_CYCAPTION) / 2 + 4;
    INPUT i[3] = {};
    for (auto& k : i) k.type = INPUT_MOUSE;
    i[0].mi.dx = x * 65535 / (GetSystemMetrics(SM_CXSCREEN) - 1);
    i[0].mi.dy = y * 65535 / (GetSystemMetrics(SM_CYSCREEN) - 1);
    i[0].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    i[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    i[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(3, i, sizeof(INPUT));
}

bool oldinga() {
    for (int i = 0; i < 5 && GetForegroundWindow() != asosiy; ++i) {
        SetForegroundWindow(asosiy);
        pompa(200);
        if (GetForegroundWindow() != asosiy) sarlavhaga_bos();
        pompa(400);
    }
    return GetForegroundWindow() == asosiy;
}

bool faollashtir(const GUID& profil) {
    HRESULT hr = pm->ActivateProfile(TF_PROFILETYPE_INPUTPROCESSOR, TIL, CLSID_OZBEK, profil, nullptr,
                                     TF_IPPMF_FORPROCESS | TF_IPPMF_DONTCARECURRENTINPUTLANGUAGE);
    pompa(400);
    TF_INPUTPROCESSORPROFILE p = {};
    bool ok = SUCCEEDED(hr) && SUCCEEDED(pm->GetActiveProfile(GUID_TFCAT_TIP_KEYBOARD, &p)) &&
              IsEqualCLSID(p.clsid, CLSID_OZBEK) && IsEqualGUID(p.guidProfile, profil);
    ok = ok && LOWORD(reinterpret_cast<ULONG_PTR>(GetKeyboardLayout(0))) == TIL;  // sxema o'zbek tilida (en-US emas)
    if (GetEnvironmentVariableW(L"JURNAL", nullptr, 0)) {
        wchar_t b[200];
        swprintf(b, 200, L"  · HKL=%08llx til=%04x profil=%08lx hr=%08lx", (unsigned long long)(ULONG_PTR)GetKeyboardLayout(0),
                 p.langid, p.guidProfile.Data1, static_cast<unsigned long>(hr));
        chiqar(b);
        pompa(1500);
        TF_INPUTPROCESSORPROFILE q = {};
        pm->GetActiveProfile(GUID_TFCAT_TIP_KEYBOARD, &q);
        swprintf(b, 200, L"  · 1,5 s keyin: HKL=%08llx til=%04x tur=%lu clsid=%08lx profil=%08lx",
                 (unsigned long long)(ULONG_PTR)GetKeyboardLayout(0), q.langid, q.dwProfileType, q.clsid.Data1,
                 q.guidProfile.Data1);
        chiqar(b);
    }
    return ok;
}

// Sozlama (xizmat Kuzatuvchi orqali keyingi tugmada o'qiydi)
bool oqi(const wchar_t* kalit, bool standart) {
    DWORD v = 0, hajm = sizeof v;
    if (RegGetValueW(HKEY_CURRENT_USER, REESTR, kalit, RRF_RT_REG_DWORD, nullptr, &v, &hajm) != ERROR_SUCCESS)
        return standart;
    return v != 0;
}

void sozla(const wchar_t* kalit, bool v) {
    DWORD d = v ? 1 : 0;
    RegSetKeyValueW(HKEY_CURRENT_USER, REESTR, kalit, REG_DWORD, &d, sizeof d);
    pompa(100);
}

// -- panel menyusini yig'uvchi soxta ITfMenu --
struct Band {
    UINT id;
    DWORD bayroq;
    std::wstring nom;
};
class Menyu final : public ITfMenu_ {
public:
    std::vector<Band> bandlar;
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_ITfMenu_) {
            *ppv = this;
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return 1; }
    STDMETHODIMP_(ULONG) Release() override { return 1; }
    STDMETHODIMP AddMenuItem(UINT id, DWORD bayroq, HBITMAP, HBITMAP, const WCHAR* pch, ULONG cch, ITfMenu_** ichki) override {
        bandlar.push_back({id, bayroq, pch ? std::wstring(pch, cch) : L""});
        if (ichki) *ichki = nullptr;
        return S_OK;
    }
};

std::wstring menyu_matni(ITfLangBarItemButton_* b) {
    Menyu m;
    b->InitMenu(&m);
    std::wstring s;
    for (const auto& x : m.bandlar) {
        if (!s.empty()) s += L" | ";
        if (x.bayroq & TF_LBMENUF_SEPARATOR)
            s += L"—";
        else
            s += (x.bayroq & TF_LBMENUF_CHECKED ? L"[x] " : L"") + x.nom;
    }
    return s;
}

std::wstring bstr(BSTR b) {
    std::wstring s = b ? b : L"";
    SysFreeString(b);
    return s;
}

bool menyu_korindi = false;
void CALLBACK menyuni_yop(HWND, UINT, UINT_PTR id, DWORD) {
    HWND m = FindWindowW(L"#32768", nullptr);
    menyu_korindi = m && IsWindowVisible(m);
    KillTimer(nullptr, id);
    EndMenu();
}

DWORD jarayon_bor(const wchar_t* nom) {
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W p = {};
    p.dwSize = sizeof p;
    DWORD id = 0;
    for (BOOL b = Process32FirstW(s, &p); b; b = Process32NextW(s, &p))
        if (!_wcsicmp(p.szExeFile, nom)) id = p.th32ProcessID;
    CloseHandle(s);
    return id;
}

void profillar() {
    struct {
        const GUID* g;
        const wchar_t* nom;
    } royxat[] = {{&PROFIL_OZ, L"ÖZ"}, {&PROFIL_ESKI, L"O'Z"}, {&PROFIL_KIRILL, L"ЎЗ"}};
    for (const auto& r : royxat) {
        TF_INPUTPROCESSORPROFILE p = {};
        HRESULT hr = pm->GetProfile(TF_PROFILETYPE_INPUTPROCESSOR, TIL, CLSID_OZBEK, *r.g, nullptr, &p);
        tekshir(SUCCEEDED(hr) && (p.dwFlags & TF_IPP_FLAG_ENABLED), std::wstring(L"profil ro'yxatda va yoqiq: ") + r.nom,
                L"hr=" + std::to_wstring(static_cast<long>(hr)));
    }
}

void yozish(HWND h, const wchar_t* qayer) {
    const std::wstring q = std::wstring(L" [") + qayer + L"]";
    tozala(h);  // odamdek: avval maydon tanlanadi, keyin rejim
    tekshir(faollashtir(PROFIL_OZ), L"ÖZ faollashdi" + q);
    tozala(h), yoz(L"shahar choy o'g'il ");
    solishtir(L"ÖZ: eski lotin → yangi" + q, L"şahar çoy öğil ", matn(h));
    tozala(h), yoz(L"wc[] ");
    solishtir(L"ÖZ: W C [ ] tugmalari" + q, L"şçöğ ", matn(h));
    tozala(h), tugma('W', true), tugma('C', true), tugma(VK_OEM_4, true), tugma(VK_OEM_6, true), yoz(L" ");
    solishtir(L"ÖZ: Shift bilan" + q, L"ŞÇÖĞ ", matn(h));
    tozala(h), tugma('W', false, true), tugma('C', false, true), tugma(VK_OEM_4, false, true), yoz(L" ");
    solishtir(L"ÖZ: o'ng Alt bilan asl belgi" + q, L"wc[ ", matn(h));
    tozala(h), yoz(L"kas\nshahar ");
    std::wstring m = matn(h);  // RichEdit qatorni «\r», EDIT «\r\n» bilan beradi
    for (size_t i; (i = m.find(L"\r\n")) != std::wstring::npos;) m.replace(i, 2, L"\r");
    solishtir(L"ÖZ: kutayotgan harfdan keyin Enter yo'qolmaydi" + q, L"kas\rşahar ", m);
    tozala(h), yoz(L"www.shifo.uz info@choy.uz ");
    solishtir(L"ÖZ: havola va email o'zgarmaydi" + q, L"www.shifo.uz info@choy.uz ", matn(h));
    sozla(L"lotin", false);
    tozala(h), yoz(L"shahar ");
    solishtir(L"ÖZ: «Eski lotindan o'girish» o'chiq" + q, L"shahar ", matn(h));
    sozla(L"lotin", true);

    tekshir(faollashtir(PROFIL_ESKI), L"O'Z faollashdi" + q);
    tozala(h), yoz(L"o'g'il shahar ");
    solishtir(L"O'Z: eski lotin o'zgarmaydi" + q, L"o'g'il shahar ", matn(h));
    sozla(L"eski_belgi", true);
    tozala(h), yoz(L"o'g'il ");
    solishtir(L"O'Z: oʻ va gʻ belgisi" + q, L"oʻgʻil ", matn(h));
    sozla(L"eski_belgi", false);

    tekshir(faollashtir(PROFIL_KIRILL), L"ЎЗ faollashdi" + q);
    tozala(h), yoz_kirill(L"шаҳар ўғил Ўзбекистон ");
    solishtir(L"ЎЗ: kirill → yangi lotin" + q, L"şahar öğil Özbekiston ", matn(h));
    sozla(L"kirill", false);
    tozala(h), yoz_kirill(L"шаҳар ");
    solishtir(L"ЎЗ: o'girish o'chiq — oddiy kirill" + q, L"шаҳар ", matn(h));
    sozla(L"kirill", true);
}

// Fokus maydonlar orasida almashganda xizmat ishlashda davom etadi
void fokus(HWND a, HWND b) {
    faollashtir(PROFIL_OZ);
    tozala(a), yoz(L"wc ");
    solishtir(L"fokus: birinchi maydon", L"şç ", matn(a));
    tozala(b), yoz(L"wc ");
    solishtir(L"fokus: ikkinchi maydonga o'tgach", L"şç ", matn(b));
    tozala(a), yoz(L"wc ");
    solishtir(L"fokus: birinchisiga qaytgach", L"şç ", matn(a));
}

void qaytarish(HWND h, const wchar_t* qayer) {
    const std::wstring q = std::wstring(L" [") + qayer + L"]";
    faollashtir(PROFIL_OZ);
    tozala(h), yoz(L"shahar \bshahar ");
    solishtir(L"ÖZ: Backspace qaytaradi va o'rganadi" + q, L"shaharshahar ", matn(h));
}

void panel() {
    ITfThreadMgr* tm = nullptr;
    ITfLangBarItemMgr* lm = nullptr;
    ITfLangBarItem* item = nullptr;
    ITfLangBarItemButton_* b = nullptr;
    CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, reinterpret_cast<void**>(&tm));
    if (tm) tm->QueryInterface(IID_ITfLangBarItemMgr, reinterpret_cast<void**>(&lm));
    faollashtir(PROFIL_OZ);
    if (lm) lm->GetItem(GUID_LBI_INPUTMODE_, &item);
    if (item) item->QueryInterface(IID_ITfLangBarItemButton_, reinterpret_cast<void**>(&b));
    TF_LANGBARITEMINFO info = {};
    if (item) item->GetInfo(&info);
    tekshir(b && IsEqualCLSID(info.clsidService, CLSID_OZBEK), L"panel tugmasi ro'yxatda (GUID_LBI_INPUTMODE)");
    if (b) {
        tekshir((info.dwStyle & TF_LBI_STYLE_SHOWNINTRAY) != 0, L"panel tugmasi tray'da ko'rinadi");
        struct {
            const GUID* g;
            const wchar_t* qisqa;
            const wchar_t* menyu;
        } kut[] = {
            {&PROFIL_OZ, L"ÖZ", L"[x] Yangi harflar tugmalarda | [x] Eski lotindan o'girish | — | Sozlamalar…"},
            {&PROFIL_ESKI, L"O'Z", L"oʻ va gʻ belgisi | — | Sozlamalar…"},
            {&PROFIL_KIRILL, L"ЎЗ", L"[x] Yangi lotinga o'girish | — | Sozlamalar…"},
        };
        for (const auto& k : kut) {
            faollashtir(*k.g);
            BSTR t = nullptr;
            b->GetText(&t);
            solishtir(std::wstring(L"panel matni ") + k.qisqa, k.qisqa, bstr(t));
            HICON ik = nullptr;
            tekshir(SUCCEEDED(b->GetIcon(&ik)) && ik, std::wstring(L"panel belgisi ") + k.qisqa);
            if (ik) DestroyIcon(ik);
            solishtir(std::wstring(L"panel menyusi ") + k.qisqa, k.menyu, menyu_matni(b));
        }
        faollashtir(PROFIL_OZ);
        b->OnMenuSelect(2);
        tekshir(!oqi(L"lotin", true), L"menyu: «Eski lotindan o'girish» o'chirildi (reestr)");
        solishtir(L"menyu: belgi olindi", L"[x] Yangi harflar tugmalarda | Eski lotindan o'girish | — | Sozlamalar…",
                  menyu_matni(b));
        HWND rich = GetDlgItem(asosiy, 2);
        tozala(rich), yoz(L"shahar ");
        solishtir(L"menyu: o'chirilgach o'girmaydi", L"shahar ", matn(rich));
        b->OnMenuSelect(2);
        tozala(rich), yoz(L"shahar ");
        solishtir(L"menyu: qayta yoqilgach o'giradi", L"şahar ", matn(rich));

        // Tray'dagi bosish: menyu ko'rinadi va tashqaridan yopiladi
        RECT r;
        GetWindowRect(asosiy, &r);
        SetTimer(nullptr, 0, 700, menyuni_yop);
        b->OnClick(TF_LBI_CLK_RIGHT, POINT{r.left + 50, r.top + 50}, &r);
        tekshir(menyu_korindi, L"bosilganda menyu ochildi va yopildi");
        oldinga();
        tekshir(GetForegroundWindow() == asosiy, L"menyudan keyin fokus dasturda");

        b->OnMenuSelect(100);
        DWORD id = 0;
        for (int i = 0; i < 40 && !(id = jarayon_bor(L"Sozlamalar.exe")); ++i) pompa(250);
        tekshir(id != 0, L"menyu: «Sozlamalar…» oynani ochdi");
        if (id) {
            pompa(3000);
            HANDLE p = OpenProcess(PROCESS_TERMINATE, FALSE, id);
            if (p) TerminateProcess(p, 0), CloseHandle(p);
        }
        b->Release();
    }
    if (item) item->Release();
    if (lm) lm->Release();
    if (tm) tm->Release();
}

}  // namespace

// tsf_sinov [guruh] — guruh berilsa faqat u yuradi: altgr, fokus (tashxis uchun)
int main(int argc, char** argv) {
    const std::string guruh = argc > 1 ? argv[1] : "";
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    us = LoadKeyboardLayoutW(L"00000409", KLF_NOTELLSHELL);
    CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfileMgr,
                     reinterpret_cast<void**>(&pm));
    if (!pm) {
        chiqar(L"✗ ITfInputProcessorProfileMgr yo'q");
        return 2;
    }
    chiqar(std::wstring(L"jarayon: ") + (sizeof(void*) == 8 ? L"64-bit" : L"32-bit"));
    RegDeleteTreeW(HKEY_CURRENT_USER, REESTR);
    if (GetEnvironmentVariableW(L"JURNAL", nullptr, 0)) sozla(L"jurnal", true);  // xizmat tashxisi
    profillar();

    LoadLibraryW(L"msftedit.dll");
    WNDCLASSW s = {};
    s.lpfnWndProc = DefWindowProcW;
    s.hInstance = GetModuleHandleW(nullptr);
    s.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    s.lpszClassName = L"OzbekSinov";
    RegisterClassW(&s);
    asosiy = CreateWindowExW(0, L"OzbekSinov", L"Özbek klaviaturasi — sinov", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 80, 80, 720,
                             420, nullptr, nullptr, s.hInstance, nullptr);
    HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL, 10, 10,
                                680, 170, asosiy, reinterpret_cast<HMENU>(1), s.hInstance, nullptr);
    HWND rich = CreateWindowExW(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL,
                                10, 190, 680, 170, asosiy, reinterpret_cast<HMENU>(2), s.hInstance, nullptr);
    SendMessageW(rich, EM_SETEDITSTYLE, SES_USECTF, SES_USECTF);
    HFONT f = CreateFontW(-20, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    SendMessageW(edit, WM_SETFONT, reinterpret_cast<WPARAM>(f), TRUE);
    SendMessageW(rich, WM_SETFONT, reinterpret_cast<WPARAM>(f), TRUE);
    pompa(500);
    tekshir(oldinga(), L"sinov oynasi oldinda (SendInput shu oynaga boradi)");
    if (GetForegroundWindow() != asosiy) {
        chiqar(L"NATIJA " + std::to_wstring(otdi) + L"/" + std::to_wstring(jami));
        return 1;
    }

    if (guruh == "altgr") {
        for (HWND h : {rich, edit}) {
            tozala(h);
            faollashtir(PROFIL_OZ);
            tozala(h), tugma('W', false, true), yoz(L" ");
            solishtir(L"o'ng Alt + W", L"w ", matn(h));
        }
    } else if (guruh == "fokus") {
        fokus(rich, edit);
    } else {
        yozish(rich, L"RichEdit, TSF");
        yozish(edit, L"EDIT, IMM32");
        fokus(rich, edit);
        panel();
        qaytarish(rich, L"RichEdit, TSF");
        qaytarish(edit, L"EDIT, IMM32");
    }

    RegDeleteTreeW(HKEY_CURRENT_USER, REESTR);
    wchar_t lugat[MAX_PATH];
    ExpandEnvironmentStringsW(L"%APPDATA%\\YangiKlaviatura\\lugat.txt", lugat, MAX_PATH);
    DeleteFileW(lugat);
    DestroyWindow(asosiy);
    pm->Release();
    chiqar(L"NATIJA " + std::to_wstring(otdi) + L"/" + std::to_wstring(jami));
    return otdi == jami ? 0 : 1;
}
