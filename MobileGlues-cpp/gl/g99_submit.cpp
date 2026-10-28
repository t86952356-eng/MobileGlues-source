// SPDX-License-Identifier: LGPL-2.1-only
// CPU command upload elision, NOT draw merging or GPU-command readback.
#include "g99_render_stats.h"
#include "buffer.h"
#include "../egl/context.h"
#include "../config/settings.h"
#include "../gles/loader.h"
#include <limits>

bool mg_g99_try_direct_elements(unsigned mode, const int* counts, unsigned type,
    const void* const* indices, int primcount, const int* basevertex) {
    // Keep true MDI, unknown contexts, CPU index pointers and manual OFF intact.
    if (!global_settings.g99_direct_submit || !g_current_ctx ||
        g_gles_caps.GL_EXT_multi_draw_indirect || primcount <= 0 ||
        !counts || !indices || mg_driver_bound_buffer(GL_ELEMENT_ARRAY_BUFFER) == 0) return false;
    const unsigned width = type == GL_UNSIGNED_BYTE ? 1 :
        type == GL_UNSIGNED_SHORT ? 2 : type == GL_UNSIGNED_INT ? 4 : 0;
    if (!width) return false;
    // Prove availability for the WHOLE batch before issuing any draw, so a
    // fallback cannot draw a prefix twice. Caller already handles restart.
    for (int i=0; i<primcount; ++i) {
        if (counts[i] < 0) return false;
        if (!counts[i]) continue;
        const uintptr_t off=reinterpret_cast<uintptr_t>(indices[i]);
        if (off % width || off / width > std::numeric_limits<GLuint>::max()) return false;
        if (basevertex && basevertex[i] != 0) {
            const bool core = g_gles_caps.major > 3 ||
                (g_gles_caps.major == 3 && g_gles_caps.minor >= 2);
            if (!GLES.glDrawElementsBaseVertex || (!core &&
                !g_gles_caps.GL_OES_draw_elements_base_vertex &&
                !g_gles_caps.GL_EXT_draw_elements_base_vertex)) return false;
        } else if (!GLES.glDrawElements) return false;
    }
    auto& stats=g_current_ctx->g99_render;
    ++stats.direct_batches;
    // Original indirect struct is 5 uint32 words, including empty subdraws.
    stats.avoided_command_bytes += uint64_t(primcount)*20;
    for (int i=0; i<primcount; ++i) {
        if (!counts[i]) continue;
        const int bv=basevertex ? basevertex[i] : 0;
        if (bv) GLES.glDrawElementsBaseVertex(mode,counts[i],type,indices[i],bv);
        else GLES.glDrawElements(mode,counts[i],type,indices[i]);
        ++stats.direct_subdraws;
    }
    return true;
}
