// Til panelidagi rejim tugmasi: tray'da «ÖZ», «O'Z» yoki «ЎЗ», bosilganda shu rejimning toggle'lari
// va «Sozlamalar…» (Linux'dagi IBus panel menyusi bilan bir xil — linux/ibus/engine.py PANEL).
#pragma once

#include <ctfutb.h>

#include "umumiy.hpp"

// MinGW sarlavhalarida yo'q e'lonlar (Windows SDK ctfutb.h dagi qiymatlar)
#ifndef TF_LBI_STYLE_SHOWNINTRAY
#define TF_LBI_STYLE_SHOWNINTRAY 0x00000002
#define TF_LBI_STYLE_BTN_BUTTON 0x00010000
#endif
#ifndef TF_LBI_ICON
#define TF_LBI_ICON 0x00000001
#define TF_LBI_TEXT 0x00000002
#define TF_LBI_TOOLTIP 0x00000004
#endif
#ifndef TF_LBMENUF_CHECKED
#define TF_LBMENUF_CHECKED 0x00000001
#define TF_LBMENUF_SEPARATOR 0x00000004
#endif

namespace xizmat {

enum TfLBIClick { TF_LBI_CLK_RIGHT = 1, TF_LBI_CLK_LEFT = 2 };

// {6F8A98E4-AAA0-4F15-8C5B-07E0DF0A3DD8}
constexpr IID IID_ITfMenu_ = {0x6f8a98e4, 0xaaa0, 0x4f15, {0x8c, 0x5b, 0x07, 0xe0, 0xdf, 0x0a, 0x3d, 0xd8}};
// {28C7F1D0-DE25-11D2-AFDD-00105A2799B5}
constexpr IID IID_ITfLangBarItemButton_ = {0x28c7f1d0, 0xde25, 0x11d2, {0xaf, 0xdd, 0x00, 0x10, 0x5a, 0x27, 0x99, 0xb5}};
// {2C77A81E-41CC-4178-A3A7-5F8A987568E6} — Windows 8+ tray'dagi kiritish rejimi belgisi
constexpr GUID GUID_LBI_INPUTMODE_ = {0x2c77a81e, 0x41cc, 0x4178, {0xa3, 0xa7, 0x5f, 0x8a, 0x98, 0x75, 0x68, 0xe6}};

struct ITfMenu_ : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE AddMenuItem(UINT id, DWORD bayroq, HBITMAP rasm, HBITMAP niqob, const WCHAR* matn,
                                                  ULONG uzunlik, ITfMenu_** ichki) = 0;
};

struct ITfLangBarItemButton_ : public ITfLangBarItem {
    virtual HRESULT STDMETHODCALLTYPE OnClick(TfLBIClick bosish, POINT nuqta, const RECT* joy) = 0;
    virtual HRESULT STDMETHODCALLTYPE InitMenu(ITfMenu_* menyu) = 0;
    virtual HRESULT STDMETHODCALLTYPE OnMenuSelect(UINT id) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetIcon(HICON* ikonka) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetText(BSTR* matn) = 0;
};

// Tugma xizmatdan rejimni so'raydi va sozlama o'zgarganini aytadi
class PanelEgasi {
public:
    virtual ozbek::Rejim panel_rejimi() = 0;
    virtual void panel_sozladi() = 0;

protected:
    ~PanelEgasi() = default;
};

class PanelTugma final : public ITfLangBarItemButton_, public ITfSource {
public:
    explicit PanelTugma(PanelEgasi* egasi);
    ~PanelTugma();
    void uzil() { egasi_ = nullptr; }  // xizmat o'chdi; TSF tugmani hali ushlab turgan bo'lishi mumkin
    void yangila();                    // rejim o'zgardi: belgi, matn va izoh qayta so'raladi

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&ref_); }
    STDMETHODIMP_(ULONG) Release() override;

    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO* info) override;
    STDMETHODIMP GetStatus(DWORD* holat) override;
    STDMETHODIMP Show(BOOL) override { return E_NOTIMPL; }
    STDMETHODIMP GetTooltipString(BSTR* izoh) override;

    STDMETHODIMP OnClick(TfLBIClick bosish, POINT nuqta, const RECT* joy) override;
    STDMETHODIMP InitMenu(ITfMenu_* menyu) override;
    STDMETHODIMP OnMenuSelect(UINT id) override;
    STDMETHODIMP GetIcon(HICON* ikonka) override;
    STDMETHODIMP GetText(BSTR* matn) override;

    STDMETHODIMP AdviseSink(REFIID riid, IUnknown* sink, DWORD* kuki) override;
    STDMETHODIMP UnadviseSink(DWORD kuki) override;

private:
    ozbek::Rejim rejim() const;
    void menyu_korsat(POINT nuqta, const RECT* joy);
    void tanla(UINT id);

    LONG ref_ = 1;
    PanelEgasi* egasi_;
    ITfLangBarItemSink* sink_ = nullptr;
    HWND oyna_ = nullptr;  // menyu egasi (ko'rinmas)
};

}  // namespace xizmat
