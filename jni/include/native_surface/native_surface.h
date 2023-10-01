#ifndef NATIVESURFACE_NATIVE_SURFACE_H
#define NATIVESURFACE_NATIVE_SURFACE_H
#include <iostream>
#include <cstdlib>
#include <android/api-level.h>
#include "utils.h"
#include "aosp_9/native_surface_9.h"
#include "aosp_10/native_surface_10.h"
#include "aosp_11/native_surface_11.h"
#include "aosp_12/native_surface_12.h"
#include "aosp_12_1/native_surface_12_1.h"
#include "aosp_13/native_surface_13.h"
void* get_createNativeWindow();
#endif
