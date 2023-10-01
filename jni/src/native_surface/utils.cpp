#include <native_surface/utils.h>

string exec_native_surface(string command) {
    char buffer[128];
    string result;
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "popen failed!";
    }
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != nullptr) {
            result += buffer;
        }
    }
    pclose(pipe);
    return result;
}

void *fdlopen(int fd, int mode) {
    char BUF[PATH_MAX];
    if (isatty(fd)) {
        errno = EBADFD;
        return nullptr;
    }
    if (access("/proc", F_OK) < 0) {
        errno = ENOSYS;
        return nullptr;
    }
    snprintf(BUF, PATH_MAX, "/proc/self/fd/%d", fd);
    if (access(BUF, F_OK) < 0) {
        errno = EBADF;
        return nullptr;
    } else if (access(BUF, R_OK) < 0) {
        errno = EACCES;
        return nullptr;
    }
    struct stat st{};
    stat(BUF, &st);
    if (S_ISDIR(st.st_mode)) {
        errno = EISDIR;
        return nullptr;
    }
    return dlopen(BUF, mode);
}



void *dlblob(const void *blob, size_t len) {
    int fd = shm_open_anon();
    ftruncate(fd, len);
    void *mem = mmap(nullptr, len, PROT_WRITE, MAP_SHARED, fd, 0);
    memcpy(mem, blob, len);
    munmap(mem, len);
    void *so = fdlopen(fd, RTLD_LAZY);
    close(fd);
    return so;
}



int get_android_api_level() {
    char prop_value[PROP_VALUE_MAX];
    __system_property_get("ro.build.version.sdk", prop_value);
    return atoi(prop_value);
}
