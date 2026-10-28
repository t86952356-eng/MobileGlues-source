// LGPL-2.1-only. Filter state assignments at the GLES dispatch boundary.
// Every MobileGlues internal GLES.* caller passes through the same interception,
// including temporary clear/FSR state and restoration. Draws are NEVER skipped.
#include "g99_state_cache.h"
#include "../gles/loader.h"
#include "../egl/context.h"
#include "../config/settings.h"
#include <cmath>

namespace {
#define RAW(name) name##_PTR real_##name = nullptr
RAW(glViewport); RAW(glScissor); RAW(glDepthMask); RAW(glDepthFunc);
RAW(glCullFace); RAW(glFrontFace); RAW(glClearColor); RAW(glClearDepthf); RAW(glClearStencil);
#undef RAW
bool installed = false;
bool forward(G99StateCache::Slot slot, std::array<uint32_t, 4> key, bool valid = true) {
    // No shared fallback shadow: untracked contexts always reach the driver.
    return !g_current_ctx || g_current_ctx->g99_state.should_forward(slot, key, valid);
}
void APIENTRY_GLES viewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    if (forward(G99StateCache::Viewport, {uint32_t(x), uint32_t(y), uint32_t(w), uint32_t(h)}, w >= 0 && h >= 0))
        real_glViewport(x, y, w, h);
}
void APIENTRY_GLES scissor(GLint x, GLint y, GLsizei w, GLsizei h) {
    if (forward(G99StateCache::Scissor, {uint32_t(x), uint32_t(y), uint32_t(w), uint32_t(h)}, w >= 0 && h >= 0))
        real_glScissor(x, y, w, h);
}
void APIENTRY_GLES depth_mask(GLboolean flag) {
    if (forward(G99StateCache::DepthMask, {uint32_t(flag), 0, 0, 0})) real_glDepthMask(flag);
}
void APIENTRY_GLES depth_func(GLenum func) {
    if (forward(G99StateCache::DepthFunc, {func, 0, 0, 0}, func >= GL_NEVER && func <= GL_ALWAYS)) real_glDepthFunc(func);
}
void APIENTRY_GLES cull_face(GLenum mode) {
    if (forward(G99StateCache::CullFace, {mode, 0, 0, 0}, mode == GL_FRONT || mode == GL_BACK || mode == GL_FRONT_AND_BACK))
        real_glCullFace(mode);
}
void APIENTRY_GLES front_face(GLenum mode) {
    if (forward(G99StateCache::FrontFace, {mode, 0, 0, 0}, mode == GL_CW || mode == GL_CCW)) real_glFrontFace(mode);
}
void APIENTRY_GLES clear_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    if (forward(G99StateCache::ClearColor,
        {G99StateCache::bits(r), G99StateCache::bits(g), G99StateCache::bits(b), G99StateCache::bits(a)},
        std::isfinite(r) && std::isfinite(g) && std::isfinite(b) && std::isfinite(a))) real_glClearColor(r, g, b, a);
}
void APIENTRY_GLES clear_depth(GLfloat depth) {
    if (forward(G99StateCache::ClearDepth, {G99StateCache::bits(depth), 0, 0, 0}, std::isfinite(depth))) real_glClearDepthf(depth);
}
void APIENTRY_GLES clear_stencil(GLint value) {
    if (forward(G99StateCache::ClearStencil, {uint32_t(value), 0, 0, 0})) real_glClearStencil(value);
}
} // namespace

void mg_g99_install_dispatch() {
    if (installed || !global_settings.helio_g99_active) return;
    installed = true;
#define INSTALL(name, wrapper) if (GLES.name) { real_##name = GLES.name; GLES.name = wrapper; }
    INSTALL(glViewport, viewport); INSTALL(glScissor, scissor);
    INSTALL(glDepthMask, depth_mask); INSTALL(glDepthFunc, depth_func);
    INSTALL(glCullFace, cull_face); INSTALL(glFrontFace, front_face);
    INSTALL(glClearColor, clear_color); INSTALL(glClearDepthf, clear_depth); INSTALL(glClearStencil, clear_stencil);
#undef INSTALL
    LOG_V("[G99 FastPath] 9 driver-state filters installed; draws, synchronization and shader math unchanged");
}
void mg_g99_present_tick() {
    if (!installed || !g_current_ctx) return;
    auto& state = g_current_ctx->g99_state;
    ++state.frames;
    if (state.frames == 120 || state.frames % 600 == 0) {
        LOG_V("[G99 FastPath] ctx=%llu frames=%llu submitted=%llu forwarded=%llu skipped=%llu (not an FPS measurement)",
              g_current_ctx->id, static_cast<unsigned long long>(state.frames),
              static_cast<unsigned long long>(state.submitted), static_cast<unsigned long long>(state.forwarded),
              static_cast<unsigned long long>(state.skipped));
    }
}
