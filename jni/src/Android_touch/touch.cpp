#include <touch.h>
#include <cstdio>
#include <cstdlib>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <linux/input.h>
#include <linux/uinput.h>
#include <ctime>

using namespace std;

#define maxE 5
#define maxF 10
#define UNGRAB 0
#define GRAB 1

Vector2 point, point2;

struct touchObj {
    bool occupy = false;
    bool isTmpDown = false;
    bool isDown = false;
    bool isUp = false;
    int x = 0;
    int y = 0;
    int id = 0;
    int size1 = 0;
    int size2 = 0;
    int size3 = 0;
};

struct targ {
    int fdNum;
    float S2TX;
    float S2TY;
};

Vector2 touch_screen_size;
float touch_s;

bool isFirstDown = true;
static pthread_t touchloop;
int fdNum = 0, origfd[maxE], nowfd;
struct touchObj Finger[maxE][maxF];
static struct input_event event[128];
static struct input_event downEvent[2];

char *_genRandomString(int length) {
    int flag, i;
    srand((unsigned) time(nullptr));
    char *tmpString = (char *) malloc(length * sizeof(char));

    for (i = 0; i < length - 1; i++) {
        flag = rand() % 3;
        switch (flag) {
            case 0:
                tmpString[i] = 'A' + rand() % 26;
                break;
            case 1:
                tmpString[i] = 'a' + rand() % 26;
                break;
            case 2:
                tmpString[i] = '0' + rand() % 10;
                break;
            default:
                tmpString[i] = 'x';
                break;
        }
    }
    tmpString[length - 1] = '\0';
    return tmpString;
}

Vector2
rotatePointx(uint32_t orientation, float x, float y, int32_t displayWidth, int32_t displayHeight) {
    if (orientation == 0) {
        return {x, y};
    }
    Vector2 xy(x, y);
    if (orientation == 3) {
        xy.x = (float) displayHeight - y;
        xy.y = x;
    } else if (orientation == 2) {
        xy.x = (float) displayWidth - x;
        xy.y = (float) displayHeight - y;
    } else if (orientation == 1) {
        xy.x = y;
        xy.y = (float) displayWidth - x;
    }
    return xy;
}

MDisplayInfo getTouchDisplyInfo() {
    if (displayInfo.orientation == 0 || displayInfo.orientation == 2) {
        return displayInfo;
    } else {
        return {displayInfo.height, displayInfo.width, displayInfo.orientation};
    }
}

void Upload() {
    int tmpCnt = 0, tmpCnt2 = 0, i, j;
    for (i = 0; i < fdNum; i++) {
        for (j = 0; j < maxF; j++) {
            if (Finger[i][j].isDown) {
                tmpCnt2++;
                if (tmpCnt2 > 10)
                    break;
                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_MT_TRACKING_ID;
                event[tmpCnt].value = Finger[i][j].id;
                tmpCnt++;
                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_X;
                event[tmpCnt].value = Finger[i][j].x;
                tmpCnt++;
                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_Y;
                event[tmpCnt].value = Finger[i][j].y;
                tmpCnt++;
                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_MT_POSITION_X;
                event[tmpCnt].value = Finger[i][j].x;
                tmpCnt++;
                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_MT_POSITION_Y;
                event[tmpCnt].value = Finger[i][j].y;
                tmpCnt++;
                if (Finger[i][j].size1) {
                    event[tmpCnt].type = EV_ABS;
                    event[tmpCnt].code = ABS_MT_TOUCH_MAJOR;
                    event[tmpCnt].value = Finger[i][j].size1;
                    tmpCnt++;
                }
                if (Finger[i][j].size2) {
                    event[tmpCnt].type = EV_ABS;
                    event[tmpCnt].code = ABS_MT_WIDTH_MAJOR;
                    event[tmpCnt].value = Finger[i][j].size2;
                    tmpCnt++;
                }
                if (Finger[i][j].size3) {
                    event[tmpCnt].type = EV_ABS;
                    event[tmpCnt].code = ABS_MT_TOUCH_MINOR;
                    event[tmpCnt].value = Finger[i][j].size3;
                    tmpCnt++;
                }
                event[tmpCnt].type = EV_SYN;
                event[tmpCnt].code = SYN_MT_REPORT;
                event[tmpCnt].value = 0;
                tmpCnt++;
            }
        }
    }
    if (tmpCnt == 0) {
        event[tmpCnt].type = EV_SYN;
        event[tmpCnt].code = SYN_MT_REPORT;
        event[tmpCnt].value = 0;
        tmpCnt++;
        if (!isFirstDown) {
            isFirstDown = true;
            event[tmpCnt].type = EV_KEY;
            event[tmpCnt].code = BTN_TOUCH;
            event[tmpCnt].value = 0;
            tmpCnt++;
            event[tmpCnt].type = EV_KEY;
            event[tmpCnt].code = BTN_TOOL_FINGER;
            event[tmpCnt].value = 0;
            tmpCnt++;
        }
    } else {
        if (isFirstDown) {
            isFirstDown = false;
            write(nowfd, downEvent, sizeof(downEvent));
        }
    }
    event[tmpCnt].type = EV_SYN;
    event[tmpCnt].code = SYN_REPORT;
    event[tmpCnt].value = 0;
    tmpCnt++;
    write(nowfd, event, sizeof(struct input_event) * tmpCnt);
}

[[noreturn]] void *TypeA(void *arg) {
    targ tmp = *(targ *) arg;
    int i = tmp.fdNum;
    float S2TX = tmp.S2TX;
    float S2TY = tmp.S2TY;
    struct input_event ie{};
    int latest = 0;
    for (;;) {
        ImGuiIO &io = ImGui::GetIO();
        if (read(origfd[i], &ie, sizeof(struct input_event))) {
            if (ie.type == EV_ABS) {
                if (ie.code == ABS_MT_SLOT) {
                    latest = ie.value;
                    continue;
                }
                if (ie.code == ABS_MT_TRACKING_ID) {
                    if (ie.value == -1) {
                        Finger[i][latest].isUp = true;
                        Finger[i][latest].isDown = false;
                        if (g_Initialized) {
                            io.MouseDown[0] = false;
                        }
                    } else {
                        Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                        Finger[i][latest].isDown = true;
                        if (g_Initialized) {
                            io.MouseDown[0] = true;
                        }
                    }
                    continue;
                }
                if (ie.code == ABS_MT_POSITION_X) {
                    Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                    Finger[i][latest].x = (int) (ie.value * S2TX);
                    Finger[i][latest].isTmpDown = true;
                    continue;
                }
                if (ie.code == ABS_MT_POSITION_Y) {
                    Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                    Finger[i][latest].y = (int) (ie.value * S2TY);
                    Finger[i][latest].isTmpDown = true;
                    continue;
                }
                if (ie.code == ABS_MT_TOUCH_MAJOR) {
                    Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                    Finger[i][latest].size1 = ie.value;
                    Finger[i][latest].isTmpDown = true;
                    continue;
                }
                if (ie.code == ABS_MT_WIDTH_MAJOR) {
                    Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                    Finger[i][latest].size2 = ie.value;
                    Finger[i][latest].isTmpDown = true;
                    continue;
                }
                if (ie.code == ABS_MT_TOUCH_MINOR) {
                    Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                    Finger[i][latest].size3 = ie.value;
                    Finger[i][latest].isTmpDown = true;
                    continue;
                }
            }
            if (g_Initialized) {
                MDisplayInfo mDisplayInfo = getTouchDisplyInfo();
                point = rotatePointx(mDisplayInfo.orientation, (float) Finger[i][latest].x,
                                     (float) Finger[i][latest].y, touch_screen_size.x,
                                     touch_screen_size.y);
                point2.x = point.x / touch_s;
                point2.y = point.y / touch_s;
                io.MousePos = ImVec2(point.x / touch_s, point.y / touch_s);
            }
            if (ie.type == EV_SYN) {
                if (ie.code == SYN_REPORT) {
                    if (Finger[i][latest].isTmpDown)
                        Upload();
                    continue;
                }
                continue;
            }
        }
    }
    return nullptr;
}

Vector2 getTouchScreenDimension1(int fd) {
    int abs_x[6], abs_y[6] = {0};
    ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), abs_x);
    ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), abs_y);
    return {(float) abs_x[2], (float) abs_y[2]};
}

int GetEventCount() {
    DIR *dir = opendir("/dev/input/");
    dirent *ptr;
    int count = 0;
    while ((ptr = readdir(dir)) != nullptr) {
        if (strstr(ptr->d_name, "event"))
            count++;
    }
    return count;
}

int GetEventId() {
    int EventCount = GetEventCount();
    int *fdArray = (int *) malloc(EventCount * 4 + 4);

    for (int i = 0; i < EventCount; i++) {
        char temp[128];
        sprintf(temp, "/dev/input/event%d", i);
        fdArray[i] = open(temp, O_RDWR | O_NONBLOCK);
    }

    input_event ev{};
    while (true) {
        for (int i = 0; i < EventCount; i++) {
            memset(&ev, 0, sizeof(ev));
            read(fdArray[i], &ev, sizeof(ev));
            if (ev.type == EV_ABS) {
                //LOGI("id:%d\n", i);
                free(fdArray);
                return i;
            }
        }
        usleep(100);
    }
}

void Touch_Init(int *retX, int *retY) {
    downEvent[0].type = EV_KEY;
    downEvent[0].code = BTN_TOUCH;
    downEvent[0].value = 1;
    downEvent[1].type = EV_KEY;
    downEvent[1].code = BTN_TOOL_FINGER;
    downEvent[1].value = 1;
    char temp[128];
    DIR *dir = opendir("/dev/input/");
    dirent *ptr;
    int eventCount = 0;
    while ((ptr = readdir(dir)) != nullptr) {
        if (strstr(ptr->d_name, "event"))
            eventCount++;
    }
    struct input_absinfo abs{}, absX[maxE], absY[maxE];
    int fd, i, tmp1, tmp2;
    int screenX, screenY, minCnt = eventCount + 1;
    fdNum = 0;
    for (i = 0; i <= eventCount; i++) {
        sprintf(temp, "/dev/input/event%d", i);
        fd = open(temp, O_RDWR);
        if (fd) {
            uint8_t *bits = nullptr;
            ssize_t bits_size = 0;
            int res, j, k;
            bool itmp1 = false, itmp2 = false, itmp3 = false;
            while (true) {
                res = ioctl(fd, EVIOCGBIT(EV_ABS, bits_size), bits);
                if (res < bits_size)
                    break;
                bits_size = res + 16;
                bits = (uint8_t *) realloc(bits, bits_size * 2);
                if (bits == nullptr) {
                    while (true) {
                        exit(0);
                    }
                }
            }
            for (j = 0; j < res; j++) {
                for (k = 0; k < 8; k++)
                    if (bits[j] & 1 << k && ioctl(fd, EVIOCGABS(j * 8 + k), &abs) == 0) {
                        if (j * 8 + k == ABS_MT_SLOT) {
                            itmp1 = true;
                            continue;
                        }
                        if (j * 8 + k == ABS_MT_POSITION_X) {
                            itmp2 = true;
                            continue;
                        }
                        if (j * 8 + k == ABS_MT_POSITION_Y) {
                            itmp3 = true;
                            continue;
                        }
                    }
            }
            if (itmp1 && itmp2 && itmp3) {
                tmp1 = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX[fdNum]);
                tmp2 = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY[fdNum]);
                if (tmp1 == 0 && tmp2 == 0) {
                    origfd[fdNum] = fd;
                    ioctl(fd, EVIOCGRAB, GRAB);
                    if (i < minCnt) {
                        screenX = absX[fdNum].maximum;
                        screenY = absY[fdNum].maximum;
                        minCnt = i;
                    }
                    fdNum++;
                    if (fdNum >= maxE)
                        break;
                }
            } else {
                close(fd);
            }
        }
    }
    if (minCnt > eventCount) {
        while (true) {
            exit(0);
        }
    }
    struct uinput_user_dev ui_dev{};
    nowfd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (nowfd <= 0) {
        while (true) {
            exit(0);
        }
    }
    memset(&ui_dev, 0, sizeof(ui_dev));
    strncpy(ui_dev.name, _genRandomString(rand() % 10 + 5), UINPUT_MAX_NAME_SIZE);
    ui_dev.id.bustype = 0;
    ui_dev.id.vendor = rand() % 10 + 5;
    ui_dev.id.product = rand() % 10 + 5;
    ui_dev.id.version = rand() % 10 + 5;
    ioctl(nowfd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);
    ioctl(nowfd, UI_SET_EVBIT, EV_ABS);
    ioctl(nowfd, UI_SET_ABSBIT, ABS_X);
    ioctl(nowfd, UI_SET_ABSBIT, ABS_Y);
    ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
    ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
    ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
    ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_TOUCH_MAJOR);
    ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_WIDTH_MAJOR);
    ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_TOUCH_MINOR);
    ioctl(nowfd, UI_SET_EVBIT, EV_SYN);
    ioctl(nowfd, UI_SET_EVBIT, EV_KEY);
    ioctl(nowfd, UI_SET_KEYBIT, BTN_TOUCH);
    ioctl(nowfd, UI_SET_KEYBIT, BTN_TOOL_FINGER);
    ioctl(nowfd, UI_SET_PHYS, _genRandomString(rand() % 10 + 5));
    sprintf(temp, "/dev/input/event%d", minCnt);
    fd = open(temp, O_RDWR);
    if (fd) {
        struct input_id id{};
        if (!ioctl(fd, EVIOCGID, &id)) {
            ui_dev.id.bustype = id.bustype;
            ui_dev.id.vendor = id.vendor;
            ui_dev.id.product = id.product;
            ui_dev.id.version = id.version;
        }
        uint8_t *bits = nullptr;
        ssize_t bits_size = 0;
        int res, j, k;
        while (true) {
            res = ioctl(fd, EVIOCGBIT(EV_KEY, bits_size), bits);
            if (res < bits_size)
                break;
            bits_size = res + 16;
            bits = (uint8_t *) realloc(bits, bits_size * 2);
            if (bits == nullptr) {
                while (true) {
                    exit(0);
                }
            }
        }
        for (j = 0; j < res; j++) {
            for (k = 0; k < 8; k++)
                if (bits[j] & 1 << k) {
                    if (j * 8 + k == BTN_TOUCH || j * 8 + k == BTN_TOOL_FINGER)
                        continue;
                    ioctl(nowfd, UI_SET_KEYBIT, j * 8 + k);
                }
        }
    }
    ui_dev.absmin[ABS_X] = 0;
    ui_dev.absmax[ABS_X] = screenX;
    ui_dev.absmin[ABS_Y] = 0;
    ui_dev.absmax[ABS_Y] = screenY;
    ui_dev.absmin[ABS_MT_POSITION_X] = 0;
    ui_dev.absmax[ABS_MT_POSITION_X] = screenX;
    ui_dev.absmin[ABS_MT_POSITION_Y] = 0;
    ui_dev.absmax[ABS_MT_POSITION_Y] = screenY;
    ui_dev.absmin[ABS_MT_TRACKING_ID] = 0;
    ui_dev.absmax[ABS_MT_TRACKING_ID] = 65535;
    ui_dev.absmin[ABS_MT_TOUCH_MAJOR] = 0;
    ui_dev.absmax[ABS_MT_TOUCH_MAJOR] = 255;
    ui_dev.absmin[ABS_MT_WIDTH_MAJOR] = 0;
    ui_dev.absmax[ABS_MT_WIDTH_MAJOR] = 255;
    ui_dev.absmin[ABS_MT_TOUCH_MINOR] = 0;
    ui_dev.absmax[ABS_MT_TOUCH_MINOR] = 255;
    *retX = screenX;
    *retY = screenY;
    write(nowfd, &ui_dev, sizeof(ui_dev));
    if (ioctl(nowfd, UI_DEV_CREATE)) {
        while (true) {
            exit(0);
        }
    }
    targ tmp[fdNum];
    for (i = 0; i < fdNum; i++) {
        tmp[i].fdNum = i;
        tmp[i].S2TX = (float) absX[i].maximum / (float) screenX;
        tmp[i].S2TY = (float) absY[i].maximum / (float) screenY;
        pthread_create(&touchloop, nullptr, TypeA, &tmp[i]);
    }
    fdNum++;
    touch_screen_size = getTouchScreenDimension1(fd); // + 1.0f;
    touch_s = (touch_screen_size.x + touch_screen_size.y) /
              (full_screen.ScreenX + full_screen.ScreenY);
}

void Touch_Down(int id, int x, int y) {
    int num = fdNum - 1;
    Finger[num][id].id = (num * 2 + 1) * maxF + id;
    Finger[num][id].x = x * touch_s;
    Finger[num][id].y = y * touch_s;
    Finger[num][id].size1 = 8;
    Finger[num][id].size2 = 8;
    Finger[num][id].size3 = 8;
    Finger[num][id].isDown = true;
    Upload();
}

void Touch_Move(int id, int x, int y) {
    int num = fdNum - 1;
    Finger[num][id].id = (num * 2 + 1) * maxF + id;
    Finger[num][id].x = x * touch_s;
    Finger[num][id].y = y * touch_s;
    Finger[num][id].size1 = 8;
    Finger[num][id].size2 = 8;
    Finger[num][id].size3 = 8;
    Finger[num][id].isDown = true;
    Upload();
}

void Touch_Up(int id) {
    int num = fdNum - 1;
    Finger[num][id].isDown = false;
    Finger[num][id].isUp = true;
    Upload();
}

// a n k i t r a w a t g i t