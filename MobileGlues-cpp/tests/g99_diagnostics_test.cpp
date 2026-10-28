// Real dispatch wrappers: counters must not modify/omit driver calls.
#include "gl/g99_state_cache.h"
#include "gl/g99_render_stats.h"
#include "egl/context.h"
#include "gles/loader.h"
#include "config/settings.h"
#include <cassert>
#include <iostream>
struct gles_func_t g_gles_func{};
struct gles_caps_t g_gles_caps{};
global_settings_t global_settings{};
thread_local MGContext* g_current_ctx=nullptr;
extern "C" void write_log(const char*, ...) {}
int __android_log_print(int,const char*,const char*,...) {return 0;}
static unsigned draw_calls=0,upload_calls=0,map_calls=0,read_calls=0,finish_calls=0;
static int marker;
static void APIENTRY_GLES draw(GLenum m,GLsizei c,GLenum t,const void* p) {assert(m==GL_TRIANGLES&&c==6&&t==GL_UNSIGNED_SHORT&&p==nullptr);++draw_calls;}
static void APIENTRY_GLES upload(GLenum t,GLintptr o,GLsizeiptr n,const void* p){assert(t==GL_ARRAY_BUFFER&&o==4&&n==8&&p==&marker);++upload_calls;}
static void* APIENTRY_GLES map(GLenum t,GLintptr o,GLsizeiptr n,GLbitfield a){assert(t==GL_ARRAY_BUFFER&&o==0&&n==4&&a==GL_MAP_READ_BIT);++map_calls;return &marker;}
static void APIENTRY_GLES read(GLint x,GLint y,GLsizei w,GLsizei h,GLenum f,GLenum t,void* p){assert(x==0&&y==1&&w==2&&h==3&&f==GL_RGBA&&t==GL_UNSIGNED_BYTE&&p==&marker);++read_calls;}
static void APIENTRY_GLES finish(){++finish_calls;}
int main(){
 GLES.glDrawElements=draw;GLES.glBufferSubData=upload;GLES.glMapBufferRange=map;GLES.glReadPixels=read;GLES.glFinish=finish;
 mg_g99_install_dispatch();assert(GLES.glDrawElements==draw);
 global_settings.helio_g99_active=true;mg_g99_install_dispatch();assert(GLES.glDrawElements!=draw);
 MGContext a{},b{};g_current_ctx=&a;
 for(int i=0;i<128;++i)GLES.glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,nullptr);
 assert(draw_calls==128&&a.g99_render.draws[G99RenderStats::Elements]==128&&a.g99_render.draw_samples==2);
 GLES.glBufferSubData(GL_ARRAY_BUFFER,4,8,&marker);assert(upload_calls==1&&a.g99_render.uploads==1&&a.g99_render.upload_bytes==8);
 assert(GLES.glMapBufferRange(GL_ARRAY_BUFFER,0,4,GL_MAP_READ_BIT)==&marker);assert(map_calls==1&&a.g99_render.maps==1&&a.g99_render.read_maps==1);
 GLES.glReadPixels(0,1,2,3,GL_RGBA,GL_UNSIGNED_BYTE,&marker);GLES.glFinish();assert(read_calls==1&&finish_calls==1&&a.g99_render.reads==1&&a.g99_render.finishes==1);
 g_current_ctx=&b;GLES.glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,nullptr);assert(draw_calls==129&&b.g99_render.draws[1]==1&&a.g99_render.draws[1]==128);
 g_current_ctx=nullptr;GLES.glFinish();GLES.glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,nullptr);assert(finish_calls==2&&draw_calls==130&&b.g99_render.draws[1]==1);
 {G99PresentSample no_context;assert(no_context.stats==nullptr);}
 g_current_ctx=&a;for(int i=0;i<599;++i){G99PresentSample sample;}assert(a.g99_render.presents==599);
 {G99PresentSample sample;}assert(a.g99_render.presents==0&&a.g99_render.draws[1]==0&&a.g99_render.upload_bytes==0&&a.g99_render.window_start>0);
 assert(b.g99_render.draws[1]==1);std::cout<<"G99 diagnostics PASS: calls and arguments forwarded, 1/64 draw sampling, window reset and context isolation preserved; no added GL flush/query/sync.\n";
}
