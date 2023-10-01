#ifndef NATIVESURFACE_EXTERN_FUNCTION_H
#define NATIVESURFACE_EXTERN_FUNCTION_H

#include <iostream>
#include <cstdlib>
#include <android/api-level.h>
#include <android/native_window.h>

struct MDisplayInfo {
    uint32_t width{0};
    uint32_t height{0};
    uint32_t orientation{0};
};

// Method pointer
struct FuncPointer {
    void *func_createNativeWindow;
    // More optional parameters
    void *func_more_createNativeWindow;
    void *func_getDisplayInfo;
};

class ExternFunction {
public:
    ExternFunction();

    /**
        * Create native surface
        * @param surface_name Create name
        * @param screen_width Create width
        * @param screen_height Create height
        * @param author whether to print author information
        */
    ANativeWindow *
    createNativeWindow(const char *surface_name, uint32_t screen_width, uint32_t screen_height,
                       bool author);

    /**
      * (More optional parameters) Create native surface
      * @param surface_name Create name
      * @param screen_width Create width
      * @param screen_height Create height
      * @param format format
      * @param flags flags
      * @param author whether to print author information
      * @return
      */
    ANativeWindow *
    createNativeWindow(const char *surface_name, uint32_t screen_width, uint32_t screen_height,
                       uint32_t format,
                       uint32_t flags, bool author);

    /**
     * Get the screen width, height and rotation status
     */
    MDisplayInfo getDisplayInfo();



};


#endif //NATIVESURFACE_EXTERN_FUNCTION_H
