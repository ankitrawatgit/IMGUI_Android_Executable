#ifndef NATIVESURFACE_DRAW_H
#define NATIVESURFACE_DRAW_H

#include <thread>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <unistd.h>
#include <fcntl.h>
#include <android/native_window.h>
#include "native_surface.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_android.h"
#include <native_surface/extern_function.h>

using namespace std;
using namespace std::chrono_literals;

extern EGLDisplay display;
extern EGLConfig config;
extern EGLSurface surface;
extern ANativeWindow *native_window;
extern EGLContext context;
extern ExternFunction externFunction;
extern MDisplayInfo displayInfo;
struct Screen {
    float ScreenX;
    float ScreenY;
};
extern int FPSg;
extern Screen full_screen;
extern int Orientation;
extern int screen_x, screen_y;
extern int init_screen_x, init_screen_y;
extern bool g_Initialized;
string exec(string command);
int init_egl(int _screen_x, int _screen_y, bool log = false);
void screen_config();
void shutdown();
void tick();
void ImGui_init();

void TouchThread();

#endif //@ankitrawatgit