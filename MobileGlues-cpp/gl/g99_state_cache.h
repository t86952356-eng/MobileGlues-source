// LGPL-2.1-only. Experimental per-context driver-state command filtering.
#ifndef MG_G99_STATE_CACHE_H
#define MG_G99_STATE_CACHE_H
#include <array>
#include <cstdint>
#include <cstring>
struct G99StateCache {
    enum Slot { Viewport, Scissor, DepthMask, DepthFunc, CullFace, FrontFace,
                ClearColor, ClearDepth, ClearStencil, Count };
    std::array<std::array<uint32_t, 4>, Count> values{};
    uint32_t known = 0;
    uint64_t submitted = 0, skipped = 0, forwarded = 0, frames = 0;
    void invalidate() { known = 0; }
    // A valid, identical state assignment has no GL side effects. Invalid
    // arguments MUST reach the driver every time so error behavior is preserved.
    bool should_forward(Slot slot, std::array<uint32_t, 4> key, bool valid = true) {
        ++submitted;
        const uint32_t bit = 1u << slot;
        if (valid && (known & bit) && values[slot] == key) {
            ++skipped;
            return false;
        }
        ++forwarded;
        if (valid) { values[slot] = key; known |= bit; }
        else { known &= ~bit; } // non-finite state may be accepted: restore must reach driver
        return true;
    }
    static uint32_t bits(float f) {
        static_assert(sizeof(float) == sizeof(uint32_t));
        uint32_t value; std::memcpy(&value, &f, sizeof(value)); return value;
    }
};
void mg_g99_install_dispatch();
void mg_g99_present_tick();
#endif
