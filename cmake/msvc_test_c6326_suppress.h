#pragma once

// Legacy standalone form of the narrow GoogleTest 1.18 C6326 false-positive
// suppression. The MSVC build now uses /wd6326 on the Release test target
// instead of force-including this file, so the Visual Studio ccache wrapper
// can cache compiler invocations without /FI.
#pragma warning(disable : 6326)
