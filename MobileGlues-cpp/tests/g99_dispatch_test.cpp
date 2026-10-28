// Links the real dispatch interceptor to a mock GLES driver (no GPU required).
#include "gl/g99_state_cache.h"
#include "egl/context.h"
#include "gles/loader.h"
#include "config/settings.h"
#include <cassert>
#include <limits>
#include <iostream>
struct gles_func_t g_gles_func{};
global_settings_t global_settings{};
thread_local MGContext* g_current_ctx = nullptr;
extern "C" void write_log(const char*, ...) {}
int __android_log_print(int, const char*, const char*, ...) { return 0; }
static int calls[9]{};
static std::array<int,4> vp{};
static void APIENTRY_GLES viewport(GLint x,GLint y,GLsizei w,GLsizei h) { ++calls[0];if(w>=0&&h>=0)vp={x,y,w,h}; }
static void APIENTRY_GLES scissor(GLint,GLint,GLsizei,GLsizei) { ++calls[1]; }
static void APIENTRY_GLES mask(GLboolean) { ++calls[2]; }
static void APIENTRY_GLES depth(GLenum) { ++calls[3]; }
static void APIENTRY_GLES cull(GLenum) { ++calls[4]; }
static void APIENTRY_GLES front(GLenum) { ++calls[5]; }
static void APIENTRY_GLES color(GLfloat,GLfloat,GLfloat,GLfloat) { ++calls[6]; }
static void APIENTRY_GLES clear_depth(GLfloat) { ++calls[7]; }
static void APIENTRY_GLES stencil(GLint) { ++calls[8]; }
int main() {
    GLES.glViewport=viewport;GLES.glScissor=scissor;GLES.glDepthMask=mask;GLES.glDepthFunc=depth;
    GLES.glCullFace=cull;GLES.glFrontFace=front;GLES.glClearColor=color;GLES.glClearDepthf=clear_depth;GLES.glClearStencil=stencil;
    mg_g99_install_dispatch();assert(GLES.glViewport==viewport); // OFF means no dispatch overhead.
    global_settings.helio_g99_active=true;mg_g99_install_dispatch();
    assert(GLES.glViewport!=viewport);mg_g99_install_dispatch(); // installation is idempotent.
    MGContext a{},b{};a.id=1;b.id=2;g_current_ctx=&a;
    for(int i=0;i<10000;++i) {
        GLES.glViewport(0,0,1080,720);GLES.glScissor(0,0,1080,720);GLES.glDepthMask(GL_TRUE);
        GLES.glDepthFunc(GL_LEQUAL);GLES.glCullFace(GL_BACK);GLES.glFrontFace(GL_CCW);
        GLES.glClearColor(0,0,0,1);GLES.glClearDepthf(1);GLES.glClearStencil(0);
    }
    for(int n:calls)assert(n==1);
    assert(a.g99_state.submitted==90000 && a.g99_state.forwarded==9 && a.g99_state.skipped==89991);
    // Internal GLES changes and their restoration use this SAME table, so they
    // update the shadow rather than silently invalidating the frontend cache.
    GLES.glViewport(4,5,320,240);assert(calls[0]==2);
    GLES.glViewport(0,0,1080,720);assert(calls[0]==3);
    assert((vp==std::array<int,4>{0,0,1080,720}));
    GLES.glViewport(0,0,-1,720);GLES.glViewport(0,0,-1,720);assert(calls[0]==5);
    GLES.glDepthFunc(0xdead);GLES.glDepthFunc(0xdead);assert(calls[3]==3);
    GLES.glCullFace(0xdead);GLES.glCullFace(0xdead);assert(calls[4]==3);
    GLES.glFrontFace(0xdead);GLES.glFrontFace(0xdead);assert(calls[5]==3);
    const auto nan=std::numeric_limits<float>::quiet_NaN();
    GLES.glClearDepthf(nan);GLES.glClearDepthf(nan);assert(calls[7]==3);
    GLES.glClearColor(nan,0,0,1);GLES.glClearColor(nan,0,0,1);assert(calls[6]==3);
    GLES.glClearDepthf(1);assert(calls[7]==4);
    GLES.glClearColor(0,0,0,1);assert(calls[6]==4);
    g_current_ctx=&b;GLES.glViewport(0,0,1080,720);assert(calls[0]==6); // distinct context first state.
    g_current_ctx=&a;a.g99_state.invalidate();GLES.glViewport(0,0,1080,720);assert(calls[0]==7);
    g_current_ctx=nullptr;GLES.glViewport(0,0,1080,720);GLES.glViewport(0,0,1080,720);assert(calls[0]==9);
    // Differential trace: cache must not reorder or lose a valid CHANGE.
    g_current_ctx=&a;uint32_t random=12345;
    for(int i=0;i<10000;++i) {
        random=random*1664525u+1013904223u;int x=int(random%100);
        auto expected=std::array<int,4>{x,0,800,600};GLES.glViewport(x,0,800,600);assert(vp==expected);
    }
    // A newly created record (even at a recycled EGL handle) starts unknown.
    MGContext recreated{};g_current_ctx=&recreated;
    int before=calls[0];GLES.glViewport(0,0,800,600);assert(calls[0]==before+1);
    std::cout<<"Real driver-dispatch tests PASS: 90000 repeated state requests -> 9 mock-driver calls; "
             <<"invalid inputs, raw internal changes, restores, context isolation, rebinds, untracked contexts and 10000 changes preserved. NOT an FPS benchmark.\n";
}
