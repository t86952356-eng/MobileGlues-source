// SPDX-License-Identifier: LGPL-2.1-only
#ifndef MG_G99_RENDER_STATS_H
#define MG_G99_RENDER_STATS_H
#include <array>
#include <chrono>
#include <cstdint>
inline uint64_t g99_now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
struct G99RenderStats {
    // Window counters, per context. No GL queries, locks or allocation in hot calls.
    enum Draw { Arrays, Elements, BaseVertex, Instanced, Indirect, DrawKinds };
    std::array<uint64_t, DrawKinds> draws{};
    uint64_t direct_batches=0, direct_subdraws=0, avoided_command_bytes=0;
    uint64_t uploads=0, upload_bytes=0, maps=0, read_maps=0, map_ns=0;
    uint64_t reads=0, read_ns=0, finishes=0, finish_ns=0;
    uint64_t draw_samples=0, draw_sample_ns=0, sample_tick=0;
    uint64_t swap_ns=0, presents=0, failures=0, window_start=0;
    int viewport_w=0, viewport_h=0;
};
// CPU-list arguments only. Never called for app/GPU-provided indirect commands.
bool mg_g99_try_direct_elements(unsigned mode, const int* counts, unsigned type,
    const void* const* indices, int primcount, const int* basevertex);
struct G99PresentSample {
    G99RenderStats* stats=nullptr;
    uint64_t start=0;
    G99PresentSample();
    ~G99PresentSample();
};
#endif
