#pragma once

#include <guiddef.h>

namespace suborno::platform::windows {

/// {E1CC38E7-F986-4371-BA1D-A527CA600677}
extern const CLSID CLSID_SubornoTextService;

/// {4B3E70D4-ED21-4C03-A909-920D404DF6EF}
extern const GUID GUID_LANGUAGE_PROFILE_SUBORNO_BENGALI;

/// Display attributes for composition text (BornoTSF parity: upstream
/// Global::BornoTSFGuidDisplayAttribute{Input,Converted}). Fresh GUIDs —
/// display-attribute GUIDs are globally registered via ITfCategoryMgr, so
/// sharing upstream's GUIDs would collide when both IMEs are installed.
/// {7A0D52E1-8C4B-4F36-9E2A-B1D3C5E7F901}
extern const GUID GUID_DISPLAY_ATTRIBUTE_INPUT;
/// {7A0D52E2-8C4B-4F36-9E2A-B1D3C5E7F901}
extern const GUID GUID_DISPLAY_ATTRIBUTE_CONVERTED;

}  // namespace suborno::platform::windows
