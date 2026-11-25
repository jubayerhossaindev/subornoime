#pragma once

// Unicode injection primitives for the standalone typing host.
//
// The standalone mode never touches TSF: it rewrites what the user typed by
// injecting events with SendInput — the same model the popular standalone
// Bangla IMEs use. Injected events carry LLKHF_INJECTED in the low-level
// hook, so the
// host's own hook must ignore them (feedback-loop guard lives in
// keyboard_hook.cpp).

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace suborno::platform::windows::standalone::injector {

// Deletes `count` characters to the left of the caret (the latin word the
// app already displayed while the user was typing).
void send_backspaces(std::size_t count);

// Sends UTF-8 text as Unicode keyboard events (VK_PACKET pipeline). Every
// UTF-16 code unit is sent individually, so astral-plane characters — and
// any future Bengali ones — survive unchanged.
void send_text_utf8(std::string_view text_utf8);

// Sends one physical key (down + up), e.g. the space or enter that the
// commit flow swallowed.
void send_virtual_key(std::uint16_t virtual_key);

}  // namespace suborno::platform::windows::standalone::injector
