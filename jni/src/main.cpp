#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/system_properties.h>

#include "draw.h"

using namespace std;

int main(int argc, char **argv) {
    screen_config();
    init_screen_x = screen_x + screen_y;
    init_screen_y = screen_y + screen_x;
    if (!init_egl(init_screen_x, init_screen_y)) {
        exit(0);
    }
    ImGui_init();
    new std::thread(TouchThread);

    while (true) {
        tick();
    }

    shutdown();
    return 0;
}
// RWTWINDROID