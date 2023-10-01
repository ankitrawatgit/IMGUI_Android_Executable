#ifndef NATIVESURFACE_TOUCH_H
#define NATIVESURFACE_TOUCH_H

#include <cstdio>
#include <cstdlib>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <linux/input.h>
#include <linux/uinput.h>
#include <ctime>
#include <string>
#include "draw.h"

struct Vector2 {
    Vector2(float x, float y) {
        this->x = x;
        this->y = y;
    }
    Vector2() {}
    float x;
    float y;
    bool operator == (const Vector2 &t) const {
        if ( this->x == t.x && this->y == t.y ) return true;
        return false;
    }
    bool operator != (const Vector2 &t) const {
        if ( this->x != t.x || this->y != t.y ) return true;
        return false;
    }
};

extern Vector2 point;
extern Vector2 point2;
void Touch_Init(int *retX, int *retY);
void Touch_Down(int id,int x, int y);
void Touch_Move(int id,int x, int y);
void Touch_Up(int id);
int GetEventCount();
int GetEventId();
#endif
