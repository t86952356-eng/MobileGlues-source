// LGPL-2.1-only. Filter state assignments at the GLES dispatch boundary.
// Every MobileGlues internal GLES.* caller passes through the same interception,
// including temporary clear/FSR state and restoration. Draws are NEVER skipped.
#include "g99_state_cache.h"
#include "../gles/loader.h"
#include "../egl/context.h"
#include "../config/settings.h"
#include <cmath>
#include "g99_render_stats.h"

namespace {
#define RAW(name) name##_PTR real_##name = nullptr
RAW(glViewport); RAW(glScissor); RAW(glDepthMask); RAW(glDepthFunc);
RAW(glCullFace); RAW(glFrontFace); RAW(glClearColor); RAW(glClearDepthf); RAW(glClearStencil);
#undef RAW
#define SAVE(name) name##_PTR real_##name = nullptr
SAVE(glDrawArrays); SAVE(glDrawElements); SAVE(glDrawElementsBaseVertex);
SAVE(glDrawArraysInstanced); SAVE(glDrawElementsInstanced); SAVE(glDrawElementsInstancedBaseVertex);
SAVE(glDrawArraysIndirect); SAVE(glDrawElementsIndirect); SAVE(glDrawRangeElements); SAVE(glDrawRangeElementsBaseVertex);
SAVE(glBufferSubData); SAVE(glMapBufferRange); SAVE(glReadPixels); SAVE(glFinish);
#undef SAVE
bool installed = false;
bool forward(G99StateCache::Slot slot, std::array<uint32_t, 4> key, bool valid = true) {
    // No shared fallback shadow: untracked contexts always reach the driver.
    return !g_current_ctx || g_current_ctx->g99_state.should_forward(slot, key, valid);
}
void APIENTRY_GLES viewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    if (g_current_ctx && w >= 0 && h >= 0) { g_current_ctx->g99_render.viewport_w=w; g_current_ctx->g99_render.viewport_h=h; }
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
// Only one in 64 draw submissions is timed, without GL synchronization.
struct DrawSample {
    G99RenderStats* stats=nullptr; uint64_t start=0;
    explicit DrawSample(G99RenderStats::Draw kind) {
        if (!g_current_ctx) return;
        auto& s=g_current_ctx->g99_render; ++s.draws[kind];
        if ((++s.sample_tick & 63u)==0) {stats=&s;start=g99_now_ns();}
    }
    ~DrawSample() {if(stats){stats->draw_sample_ns+=g99_now_ns()-start;++stats->draw_samples;}}
};
#define DRAW_WRAPPER(name, kind, args, call) \
    void APIENTRY_GLES trace_##name args { DrawSample sample(G99RenderStats::kind); real_##name call; }
DRAW_WRAPPER(glDrawArrays,Arrays,(GLenum m,GLint f,GLsizei c),(m,f,c))
DRAW_WRAPPER(glDrawElements,Elements,(GLenum m,GLsizei c,GLenum t,const void* i),(m,c,t,i))
DRAW_WRAPPER(glDrawElementsBaseVertex,BaseVertex,(GLenum m,GLsizei c,GLenum t,const void* i,GLint b),(m,c,t,i,b))
DRAW_WRAPPER(glDrawArraysInstanced,Instanced,(GLenum m,GLint f,GLsizei c,GLsizei n),(m,f,c,n))
DRAW_WRAPPER(glDrawElementsInstanced,Instanced,(GLenum m,GLsizei c,GLenum t,const void* i,GLsizei n),(m,c,t,i,n))
DRAW_WRAPPER(glDrawElementsInstancedBaseVertex,Instanced,(GLenum m,GLsizei c,GLenum t,const void* i,GLsizei n,GLint b),(m,c,t,i,n,b))
DRAW_WRAPPER(glDrawArraysIndirect,Indirect,(GLenum m,const void* i),(m,i))
DRAW_WRAPPER(glDrawElementsIndirect,Indirect,(GLenum m,GLenum t,const void* i),(m,t,i))
DRAW_WRAPPER(glDrawRangeElements,Elements,(GLenum m,GLuint a,GLuint b,GLsizei c,GLenum t,const void* i),(m,a,b,c,t,i))
DRAW_WRAPPER(glDrawRangeElementsBaseVertex,BaseVertex,(GLenum m,GLuint a,GLuint b,GLsizei c,GLenum t,const void* i,GLint v),(m,a,b,c,t,i,v))
#undef DRAW_WRAPPER
void APIENTRY_GLES trace_upload(GLenum t,GLintptr o,GLsizeiptr n,const void* d) {
    if(g_current_ctx){auto& s=g_current_ctx->g99_render;++s.uploads;if(n>0)s.upload_bytes+=n;}
    real_glBufferSubData(t,o,n,d);
}
void* APIENTRY_GLES trace_map(GLenum t,GLintptr o,GLsizeiptr n,GLbitfield a) {
    if(!g_current_ctx)return real_glMapBufferRange(t,o,n,a);
    auto& s=g_current_ctx->g99_render;++s.maps;if(a&GL_MAP_READ_BIT)++s.read_maps;
    const auto start=g99_now_ns();void* p=real_glMapBufferRange(t,o,n,a);s.map_ns+=g99_now_ns()-start;return p;
}
void APIENTRY_GLES trace_read(GLint x,GLint y,GLsizei w,GLsizei h,GLenum f,GLenum t,void* p) {
    if(!g_current_ctx){real_glReadPixels(x,y,w,h,f,t,p);return;}
    auto& s=g_current_ctx->g99_render;++s.reads;const auto start=g99_now_ns();
    real_glReadPixels(x,y,w,h,f,t,p);s.read_ns+=g99_now_ns()-start;
}
void APIENTRY_GLES trace_finish() {
    if(!g_current_ctx){real_glFinish();return;}
    auto& s=g_current_ctx->g99_render;++s.finishes;const auto start=g99_now_ns();
    real_glFinish();s.finish_ns+=g99_now_ns()-start;
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
    INSTALL(glDrawArrays, trace_glDrawArrays);
    INSTALL(glDrawElements, trace_glDrawElements);
    INSTALL(glDrawElementsBaseVertex, trace_glDrawElementsBaseVertex);
    INSTALL(glDrawArraysInstanced, trace_glDrawArraysInstanced);
    INSTALL(glDrawElementsInstanced, trace_glDrawElementsInstanced);
    INSTALL(glDrawElementsInstancedBaseVertex, trace_glDrawElementsInstancedBaseVertex);
    INSTALL(glDrawArraysIndirect, trace_glDrawArraysIndirect);
    INSTALL(glDrawElementsIndirect, trace_glDrawElementsIndirect);
    INSTALL(glDrawRangeElements, trace_glDrawRangeElements);
    INSTALL(glDrawRangeElementsBaseVertex, trace_glDrawRangeElementsBaseVertex);
    INSTALL(glBufferSubData, trace_upload); INSTALL(glMapBufferRange, trace_map);
    INSTALL(glReadPixels, trace_read); INSTALL(glFinish, trace_finish);
#undef INSTALL
    LOG_V("[G99 Test3] CPU-command direct-submit=%d; real-MDI=%d; per-context render diagnostics enabled; no draw merging or shader changes", global_settings.g99_direct_submit, g_gles_caps.GL_EXT_multi_draw_indirect);
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

G99PresentSample::G99PresentSample() {
    if (!global_settings.helio_g99_active || !g_current_ctx) return;
    stats=&g_current_ctx->g99_render;start=g99_now_ns();
    if (!stats->window_start) stats->window_start=start;
}
G99PresentSample::~G99PresentSample() {
    if (!stats) return;
    const auto now=g99_now_ns(); stats->swap_ns+=now-start; ++stats->presents;
    if (stats->presents < 600) return;
    const double window_ms=double(now-stats->window_start)/1e6;
    LOG_V("[G99 Render] window_ms=%.1f presents=%llu viewport=%dx%d draws_arrays=%llu elements=%llu basevertex=%llu instanced=%llu indirect=%llu bypass_batches=%llu bypass_subdraws=%llu avoided_cmd_bytes=%llu",window_ms,(unsigned long long)stats->presents,stats->viewport_w,stats->viewport_h,(unsigned long long)stats->draws[0],(unsigned long long)stats->draws[1],(unsigned long long)stats->draws[2],(unsigned long long)stats->draws[3],(unsigned long long)stats->draws[4],(unsigned long long)stats->direct_batches,(unsigned long long)stats->direct_subdraws,(unsigned long long)stats->avoided_command_bytes);
    LOG_V("[G99 Wait] swap_cpu_ms=%.3f draw_samples=%llu draw_sample_cpu_ms=%.3f uploads=%llu upload_bytes=%llu maps=%llu read_maps=%llu map_cpu_ms=%.3f reads=%llu read_cpu_ms=%.3f finish_calls=%llu finish_cpu_ms=%.3f (CPU wall time; NOT GPU duration; swap includes vsync/pacing; no forced flush)",double(stats->swap_ns)/1e6,(unsigned long long)stats->draw_samples,double(stats->draw_sample_ns)/1e6,(unsigned long long)stats->uploads,(unsigned long long)stats->upload_bytes,(unsigned long long)stats->maps,(unsigned long long)stats->read_maps,double(stats->map_ns)/1e6,(unsigned long long)stats->reads,double(stats->read_ns)/1e6,(unsigned long long)stats->finishes,double(stats->finish_ns)/1e6);
    const int w=stats->viewport_w,h=stats->viewport_h;
    *stats=G99RenderStats{};stats->viewport_w=w;stats->viewport_h=h;stats->window_start=now;
}
