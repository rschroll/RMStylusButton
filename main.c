#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <time.h>

#define PEN_DEVICE      "/dev/input/event1"
#define UINPUT_DEVICE   "/dev/uinput"

// Times in seconds
#define PRESS_TIMEOUT 0.2


static int verbose = 0;
#define Vprintf verbose && printf
#define VVprintf (verbose >= 2) && printf

void writeEventVals(int fd, unsigned short type, unsigned short code, signed int value) {
    struct input_event event = (struct input_event) {.type=type, .code=code, .value=value};
    gettimeofday(&event.time, NULL);
    VVprintf("writing: seconds = %ld, usec= %ld, type = %d, code = %d, value = %d\n", event.time.tv_sec, event.time.tv_usec, event.type, event.code, event.value);
    write(fd, &event, sizeof(struct input_event));
}

void grabPen(int fd_pen) {
    // Lift the pen away from the surface, to allow multitouch to be registered.
    writeEventVals(fd_pen, EV_KEY, BTN_TOOL_PEN, 0);
    writeEventVals(fd_pen, EV_SYN, SYN_REPORT, 0);

    // Grab the pen device to keep more inputs from undoing the above.
    ioctl(fd_pen, EVIOCGRAB, 1);
}

void ungrabPen(int fd_pen) {
    // Release the pen input
    ioctl(fd_pen, EVIOCGRAB, 0);

    // Bring the pen back into proximity of the surface.
    writeEventVals(fd_pen, EV_KEY, BTN_TOOL_PEN, 1);
    writeEventVals(fd_pen, EV_SYN, SYN_REPORT, 0);
}

void writeUndoRedo(int fd_keyboard, bool redo) {
    writeEventVals(fd_keyboard, EV_SYN, SYN_REPORT, 0);
    writeEventVals(fd_keyboard, EV_KEY, KEY_LEFTCTRL, 1);
    if (redo) {
        writeEventVals(fd_keyboard, EV_KEY, KEY_LEFTSHIFT, 1);
        writeEventVals(fd_keyboard, EV_SYN, SYN_REPORT, 0);
    }
    writeEventVals(fd_keyboard, EV_KEY, KEY_Z, 1);
    writeEventVals(fd_keyboard, EV_SYN, SYN_REPORT, 0);
    writeEventVals(fd_keyboard, EV_KEY, KEY_Z, 0);
    writeEventVals(fd_keyboard, EV_SYN, SYN_REPORT, 0);
    if (redo) {
        writeEventVals(fd_keyboard, EV_KEY, KEY_LEFTSHIFT, 0);
        writeEventVals(fd_keyboard, EV_SYN, SYN_REPORT, 0);
    }
    writeEventVals(fd_keyboard, EV_KEY, KEY_LEFTCTRL, 0);
    writeEventVals(fd_keyboard, EV_SYN, SYN_REPORT, 0);
}

int createKeyboardDevice() {
    // https://stackoverflow.com/questions/40676172/how-to-generate-key-strokes-events-with-the-input-subsystem
    int fd_key_emulator;
    struct uinput_user_dev dev_fake_keyboard = {
        .name = "kb-emulator",
        .id = {.bustype = BUS_USB, .vendor = 0x01, .product = 0x01, .version = 1}
    };

    // Open the uinput device
    fd_key_emulator = open(UINPUT_DEVICE, O_WRONLY | O_NONBLOCK);
    if (fd_key_emulator < 0) {
        fprintf(stderr, "Error in opening uinput: %s\n", strerror(errno));
        return -1;
    }

    // Enable all of the events we will need
    if (ioctl(fd_key_emulator, UI_SET_EVBIT, EV_KEY)
        || ioctl(fd_key_emulator, UI_SET_KEYBIT, KEY_A)
        || ioctl(fd_key_emulator, UI_SET_KEYBIT, KEY_Z)
        || ioctl(fd_key_emulator, UI_SET_KEYBIT, KEY_LEFTSHIFT)
        || ioctl(fd_key_emulator, UI_SET_KEYBIT, KEY_LEFTCTRL)
        || ioctl(fd_key_emulator, UI_SET_EVBIT, EV_SYN)) {
        fprintf(stderr, "Error in ioctl sets: %s\n", strerror(errno));
        return -1;
    }

    // Write the uinput_user_dev structure into uinput file descriptor
    if (write(fd_key_emulator, &dev_fake_keyboard, sizeof(struct uinput_user_dev)) < 0) {
        fprintf(stderr, "Error in write(): uinput_user_dev struct into uinput file descriptor: %s\n", strerror(errno));
        return -1;
    }

    // Create the device via an IOCTL call
    if (ioctl(fd_key_emulator, UI_DEV_CREATE)) {
        fprintf(stderr, "Error in ioctl : UI_DEV_CREATE : %s\n", strerror(errno));
        return -1;
    }

    return fd_key_emulator;
}

bool laterThan(struct timeval now, struct timeval then, double delta) {
    double elapsed = (now.tv_sec - then.tv_sec) + (now.tv_usec / 1000000.0 - then.tv_usec / 1000000.0);
    return elapsed > delta;
}

void mainloop(int fd_pen, int fd_keyboard, bool toggle) {
    struct input_event ev_pen;
    const size_t ev_pen_size = sizeof(struct input_event);
    int n_clicks = 0;
    bool primed = false;
    struct timeval last_click, grab_start;
    bool eraser_on = false;

    grab_start.tv_sec = 0;

    for (;;) {
        read(fd_pen, &ev_pen, ev_pen_size); //note: read pauses until there is data

        if (ev_pen.type == EV_KEY && ev_pen.code == BTN_STYLUS) {
            if (!toggle)
                writeEventVals(fd_pen, EV_KEY, BTN_TOOL_RUBBER, ev_pen.value);
            if (ev_pen.value == 1) {  // press
                if (n_clicks != 0 && laterThan(ev_pen.time, last_click, PRESS_TIMEOUT)) {  // Outstanding event?
                    n_clicks = 0;
                    if (grab_start.tv_sec != 0) {
                        ungrabPen(fd_pen);
                        grab_start.tv_sec = 0;
                    }
                }
                n_clicks += 1;
                last_click = ev_pen.time;
                primed = false;
                if (n_clicks == 2){
                    // We are entering multi-click territory.  We'll grab the pen input
                    // so there's some time before triggering a keyboard event.
                    grabPen(fd_pen);
                    grab_start = ev_pen.time;
                }
            } else if (ev_pen.value == 0) {  // release
                if (!laterThan(ev_pen.time, last_click, PRESS_TIMEOUT)) {
                    primed = true;
                    last_click = ev_pen.time;
                } else {
                    n_clicks = 0;
                    primed = false;
                    if (grab_start.tv_sec != 0) {
                        ungrabPen(fd_pen);
                        grab_start.tv_sec = 0;
                    }
                }
            }
        } else if (primed && ev_pen.type == EV_SYN && ev_pen.code == SYN_REPORT &&
                   laterThan(ev_pen.time, last_click, PRESS_TIMEOUT)) {
            Vprintf("%ix click event detected\n", n_clicks);
            if (n_clicks == 1 && toggle) {
                eraser_on = !eraser_on;
                Vprintf("Writing eraser tool %d\n", eraser_on);
                writeEventVals(fd_pen, EV_KEY, BTN_TOOL_RUBBER, eraser_on);
                if (!eraser_on) {
                    // Setting the rubber tool off isn't enough; the pen needs to be
                    // moved away from the screen and then back.
                    writeEventVals(fd_pen, EV_KEY, BTN_TOOL_PEN, 0);
                    writeEventVals(fd_pen, EV_KEY, BTN_TOOL_PEN, 1);
                }
            } else if (n_clicks > 1) {
                writeUndoRedo(fd_keyboard, n_clicks > 2);
                ungrabPen(fd_pen);
                grab_start.tv_sec = 0;
            }
            n_clicks = 0;
            primed = false;
        } else if (eraser_on && ev_pen.type == EV_KEY && ev_pen.code == BTN_TOOL_PEN) {
            // When the pen moves away from the screen, it resets the rubber tool.  This
            // explicitly turns it off, and more importantly turns it back on.
            Vprintf("Writing eraser tool %d\n", ev_pen.value);
            writeEventVals(fd_pen, EV_KEY, BTN_TOOL_RUBBER, ev_pen.value);
        }
    }
}

int main(int argc, char *argv[]) {
    bool toggle_mode = false;
    int fd_pen, fd_keyboard;

    //check our input args
    for(int i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "--toggle", 8)) {
            toggle_mode = true;
        } else if (!strncmp(argv[i], "--verbose", 9) || !strncmp(argv[i], "-v", 2)) {
            verbose += 1;
        } else {
            printf("Unknown argument %s\nExiting...\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }
    Vprintf("RemarkableLamyEraser 1.2\n");
    if (toggle_mode)
        Vprintf("Mode: toggle\n");

    /* Open Device: Pen */
    fd_pen = open(PEN_DEVICE, O_RDWR);
    if (fd_pen == -1) {
        fprintf(stderr, "%s is not a vaild device\n", PEN_DEVICE);
        exit(EXIT_FAILURE);
    }
    /* Create Keyboard */
    fd_keyboard = createKeyboardDevice();
    if (fd_keyboard == -1) {
        fprintf(stderr, "Unable to create keyboard\n");
        exit(EXIT_FAILURE);
    }

    /* Print Device Name */
    if (verbose) {
        char name[256] = "Unknown";
        ioctl(fd_pen, EVIOCGNAME(sizeof(name)), name);
        printf("Using Devices:\n");
        printf("1. device file = %s\n", PEN_DEVICE);
        printf("   device name = %s\n", name);
        ioctl(fd_keyboard, EVIOCGNAME(sizeof(name)), name);
        printf("2. device file = %s\n", UINPUT_DEVICE);
        printf("   device name = %s\n", name);
    }

    mainloop(fd_pen, fd_keyboard, toggle_mode);
    return EXIT_SUCCESS;
}
