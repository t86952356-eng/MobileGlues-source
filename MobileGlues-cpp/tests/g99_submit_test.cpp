// Links the real CPU-command upload elision to a mock GLES driver.
#include "gl/g99_render_stats.h"
#include "gl/buffer.h"
#include "egl/context.h"
#include "config/settings.h"
#include "gles/loader.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <tuple>
struct gles_func_t g_gles_func{};
struct gles_caps_t g_gles_caps{};
global_settings_t global_settings{};
thread_local MGContext* g_current_ctx=nullptr;
static GLuint ebo=9;
GLuint mg_driver_bound_buffer(GLenum target) { assert(target==GL_ELEMENT_ARRAY_BUFFER);return ebo; }
using Draw=std::tuple<GLenum,GLsizei,GLenum,uintptr_t,GLint>;
static std::vector<Draw> trace;
static void APIENTRY_GLES draw(GLenum m,GLsizei c,GLenum t,const void* i) {trace.emplace_back(m,c,t,reinterpret_cast<uintptr_t>(i),0);}
static void APIENTRY_GLES draw_bv(GLenum m,GLsizei c,GLenum t,const void* i,GLint b) {trace.emplace_back(m,c,t,reinterpret_cast<uintptr_t>(i),b);}
int main() {
 GLES.glDrawElements=draw;GLES.glDrawElementsBaseVertex=draw_bv;
 g_gles_caps.major=3;g_gles_caps.minor=2;
 MGContext a{},b{};g_current_ctx=&a;
 const int counts[]={6,0,9,3};const int bv[]={0,100,-4,70000};
 const void* indices[]={nullptr,reinterpret_cast<void*>(12),reinterpret_cast<void*>(24),reinterpret_cast<void*>(60)};
 auto run=[&]{return mg_g99_try_direct_elements(GL_TRIANGLES,counts,GL_UNSIGNED_SHORT,indices,4,bv);};
 assert(!run()&&trace.empty()); // toggle OFF
 global_settings.g99_direct_submit=true;g_current_ctx=nullptr;assert(!run());
 g_current_ctx=&a;ebo=0;assert(!run());ebo=9;
 g_gles_caps.GL_EXT_multi_draw_indirect=1;assert(!run());g_gles_caps.GL_EXT_multi_draw_indirect=0;
 // Unsupported BV or malformed input must decline the ENTIRE batch before any draw.
 GLES.glDrawElementsBaseVertex=nullptr;assert(!run()&&trace.empty());GLES.glDrawElementsBaseVertex=draw_bv;
 GLES.glDrawElements=nullptr;assert(!run()&&trace.empty());GLES.glDrawElements=draw;
 g_gles_caps.minor=1;assert(!run());g_gles_caps.GL_OES_draw_elements_base_vertex=1;assert(run());
 assert(trace.size()==3);trace.clear();g_gles_caps.minor=2;g_gles_caps.GL_OES_draw_elements_base_vertex=0;
 int negative[]={6,0,-1,3};assert(!mg_g99_try_direct_elements(GL_TRIANGLES,negative,GL_UNSIGNED_SHORT,indices,4,bv));
 auto misaligned=indices[2];indices[2]=reinterpret_cast<void*>(25);assert(!run()&&trace.empty());indices[2]=misaligned;
 indices[3]=reinterpret_cast<void*>(uintptr_t(1)<<40);assert(!run()&&trace.empty());indices[3]=reinterpret_cast<void*>(60);
 assert(!mg_g99_try_direct_elements(GL_TRIANGLES,counts,0xdead,indices,4,bv));
 assert(!mg_g99_try_direct_elements(GL_TRIANGLES,nullptr,GL_UNSIGNED_SHORT,indices,4,bv));
 assert(!mg_g99_try_direct_elements(GL_TRIANGLES,counts,GL_UNSIGNED_SHORT,nullptr,4,bv));
 assert(!mg_g99_try_direct_elements(GL_TRIANGLES,counts,GL_UNSIGNED_SHORT,indices,0,bv));
 assert(!mg_g99_try_direct_elements(GL_TRIANGLES,counts,GL_UNSIGNED_SHORT,indices,-1,bv));
 // Randomized differential trace vs the original CPU-list indirect semantics.
 uint32_t rng=9;
 for(unsigned type:{GL_UNSIGNED_BYTE,GL_UNSIGNED_SHORT,GL_UNSIGNED_INT}) {
  unsigned width=type==GL_UNSIGNED_BYTE?1:type==GL_UNSIGNED_SHORT?2:4;
  for(int batch=0;batch<1000;++batch) {
   int c[8],v[8];const void* off[8];std::vector<Draw> expected;
   for(int j=0;j<8;++j){rng=rng*1664525u+1013904223u;c[j]=int(rng%20)*3;v[j]=(j%3==0)?0:int(rng%100000)-50000;off[j]=reinterpret_cast<void*>(uintptr_t(j*128*width));if(c[j])expected.emplace_back(GL_TRIANGLES,c[j],type,reinterpret_cast<uintptr_t>(off[j]),v[j]);}
   trace.clear();assert(mg_g99_try_direct_elements(GL_TRIANGLES,c,type,off,8,v));assert(trace==expected);
  }
 }
 trace.clear();g_current_ctx=&b;assert(run());assert(b.g99_render.direct_batches==1&&b.g99_render.direct_subdraws==3&&b.g99_render.avoided_command_bytes==80);
 assert(a.g99_render.direct_batches==3001);assert(a.g99_render.avoided_command_bytes==3000*160+80);
 trace.clear();assert(mg_g99_try_direct_elements(GL_TRIANGLES,counts,GL_UNSIGNED_SHORT,indices,4,nullptr));
 assert(std::get<4>(trace.back())==0); // plain list has no BV component
 std::cout<<"G99 direct submit PASS: 3000 randomized batches preserve draw order/count/type/offset/basevertex; OFF/real-MDI/unbound/unsupported/invalid decline before drawing; zero counts and per-context counters correct. NOT an FPS benchmark.\n";
}
