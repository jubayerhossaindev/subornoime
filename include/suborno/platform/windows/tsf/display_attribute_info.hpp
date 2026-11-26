#pragma once

// BornoTSF parity components (GPL-3.0-or-later): ported from BornoTSF
// display/DisplayAttributeInfo.{h,cpp} and display/EnumDisplayAttributeInfo.{h,cpp}
// (c) Jayed Ahsan Saad, BornoTSF contributors.
//
// What the port keeps verbatim:
//   - Two display attributes (Input / Converted), registered with the TSF
//     category manager and returned through ITfDisplayAttributeProvider.
//   - The IEnumTfDisplayAttributeInfo walk order: Input first, Converted
//     second (MAX_DISPLAY_ATTRIBUTE_INFO == 2), Clone/Reset/Skip semantics.
//   - Upstream GetAttributeInfo quirk: the static TF_DISPLAYATTRIBUTE tables
//     are overwritten before returning, so both attributes effectively render
//     as a black squiggle underline (TF_LS_SQUIGGLE, RGB(0,0,0),
//     TF_ATTR_INPUT). Kept on purpose — Suborno must render composition text
//     exactly like BornoTSF did. The upstream tables are kept as statics for
//     structural parity with the original code.
//
// What the port adapts:
//   - Global::BornoTSFGuidDisplayAttribute{Input,Converted} become Suborno's
//     own GUID_DISPLAY_ATTRIBUTE_{INPUT,CONVERTED} (fresh GUIDs — display
//     attribute GUIDs are registered globally, so two IMEs sharing the same
//     GUIDs would collide when both are installed).
//   - BornoTSF's manual DllAddRef/DllRelease becomes the shared
//     com::module_lifetime() lock used by every Suborno COM object.
//   - Prefix convention: BornoTSF "CDisplayAttributeInfo*" ->
//     Suborno "DisplayAttributeInfo*" inside suborno::platform::windows.

#include <msctf.h>
#include <string>
#include <wil/com.h>

namespace suborno::platform::windows {

/// ITfDisplayAttributeInfo for one attribute (base) — mirrors
/// BornoTSF CDisplayAttributeInfo.
class DisplayAttributeInfo : public ITfDisplayAttributeInfo {
  public:
    DisplayAttributeInfo(const GUID* guid,
                         const TF_DISPLAYATTRIBUTE* display_attribute,
                         const wchar_t* description);
    virtual ~DisplayAttributeInfo();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** object) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    // ITfDisplayAttributeInfo
    STDMETHOD(GetGUID)(_Out_ GUID* guid) override;
    STDMETHOD(GetDescription)(_Outptr_result_z_ BSTR* description) override;
    STDMETHOD(GetAttributeInfo)(_Out_ TF_DISPLAYATTRIBUTE* attribute) override;
    STDMETHOD(SetAttributeInfo)(_In_ const TF_DISPLAYATTRIBUTE* attribute) override;
    STDMETHOD(Reset)() override;

  private:
    const GUID* guid_;
    const TF_DISPLAYATTRIBUTE* display_attribute_;
    std::wstring description_;
    volatile LONG reference_count_ = 1;
};

/// The two concrete attributes BornoTSF exposes.
class DisplayAttributeInfoInput final : public DisplayAttributeInfo {
  public:
    DisplayAttributeInfoInput()
        : DisplayAttributeInfo(guid(), attribute(), description()) {}

    static const GUID* guid();
    static const TF_DISPLAYATTRIBUTE* attribute();
    static const wchar_t* description();
};

class DisplayAttributeInfoConverted final : public DisplayAttributeInfo {
  public:
    DisplayAttributeInfoConverted()
        : DisplayAttributeInfo(guid(), attribute(), description()) {}

    static const GUID* guid();
    static const TF_DISPLAYATTRIBUTE* attribute();
    static const wchar_t* description();
};

/// IEnumTfDisplayAttributeInfo over {Input, Converted} — mirrors
/// BornoTSF CEnumDisplayAttributeInfo.
class EnumDisplayAttributeInfo : public IEnumTfDisplayAttributeInfo {
  public:
    EnumDisplayAttributeInfo();
    virtual ~EnumDisplayAttributeInfo();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** object) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    // IEnumTfDisplayAttributeInfo
    STDMETHOD(Clone)(_Out_ IEnumTfDisplayAttributeInfo** enumerator) override;
    STDMETHOD(Next)(ULONG count,
                    __RPC__out_ecount_part(count, *fetched) ITfDisplayAttributeInfo** info,
                    __RPC__out ULONG* fetched) override;
    STDMETHOD(Reset)() override;
    STDMETHOD(Skip)(ULONG count) override;

  private:
    ULONG index_ = 0;
    volatile LONG reference_count_ = 1;
};

}  // namespace suborno::platform::windows
