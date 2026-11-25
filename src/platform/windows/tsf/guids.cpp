#include "suborno/platform/windows/tsf/guids.hpp"

#include <initguid.h>

namespace suborno::platform::windows {

// {E1CC38E7-F986-4371-BA1D-A527CA600677}
DEFINE_GUID(CLSID_SubornoTextService,
            0xe1cc38e7,
            0xf986,
            0x4371,
            0xba,
            0x1d,
            0xa5,
            0x27,
            0xca,
            0x60,
            0x06,
            0x77);

// {4B3E70D4-ED21-4C03-A909-920D404DF6EF}
DEFINE_GUID(GUID_LANGUAGE_PROFILE_SUBORNO_BENGALI,
            0x4b3e70d4,
            0xed21,
            0x4c03,
            0xa9,
            0x09,
            0x92,
            0x0d,
            0x40,
            0x4d,
            0xf6,
            0xef);

// {7A0D52E1-8C4B-4F36-9E2A-B1D3C5E7F901}
DEFINE_GUID(GUID_DISPLAY_ATTRIBUTE_INPUT,
            0x7a0d52e1,
            0x8c4b,
            0x4f36,
            0x9e,
            0x2a,
            0xb1,
            0xd3,
            0xc5,
            0xe7,
            0xf9,
            0x01);

// {7A0D52E2-8C4B-4F36-9E2A-B1D3C5E7F901}
DEFINE_GUID(GUID_DISPLAY_ATTRIBUTE_CONVERTED,
            0x7a0d52e2,
            0x8c4b,
            0x4f36,
            0x9e,
            0x2a,
            0xb1,
            0xd3,
            0xc5,
            0xe7,
            0xf9,
            0x01);

}  // namespace suborno::platform::windows
