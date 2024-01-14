#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <linux/input.h>
#include <time.h>

#define WIDTH  1404
#define HEIGHT 1872

#define PEN_DEVICE      "/dev/input/event1"
#define TOUCH_DEVICE    "/dev/input/event2"

// Times in seconds
#define TOUCH_PAUSE   0.55
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

void writeMultiTap(int fd_touch, int fd_pen, int n) {
    // https://www.kernel.org/doc/html/latest/input/multi-touch-protocol.html#protocol-example-b
    int tracking_id = 100,
        x = WIDTH / 3, // - panelTouch[0],
        dx = 200,
        y = HEIGHT / 2;

    if (n < 1)
        return;

    // Lift the pen away from the surface, to allow multitouch to be registered.
    writeEventVals(fd_pen, EV_KEY, BTN_TOOL_PEN, 0);
    writeEventVals(fd_pen, EV_SYN, SYN_REPORT, 0);

    // Grab the pen device to keep more inputs from undoing the above.
    ioctl(fd_pen, EVIOCGRAB, 1);

    // Make sure these events get processed before simulating the multitouch.
    usleep((int)(TOUCH_PAUSE * 1000000));

    // Write n touch events.
    for (int i=0; i<n; i++) {
        writeEventVals(fd_touch, EV_ABS, ABS_MT_SLOT, i);
        writeEventVals(fd_touch, EV_ABS, ABS_MT_TRACKING_ID, tracking_id + i);
        writeEventVals(fd_touch, EV_ABS, ABS_MT_POSITION_X, x + i*dx);
        writeEventVals(fd_touch, EV_ABS, ABS_MT_POSITION_Y, y);
    }
    writeEventVals(fd_touch, EV_SYN, SYN_REPORT, 0);

    // Write n touch release events.
    for (int i=0; i<n; i++) {
        writeEventVals(fd_touch, EV_ABS, ABS_MT_SLOT, i);
        writeEventVals(fd_touch, EV_ABS, ABS_MT_TRACKING_ID, -1);
    }
    writeEventVals(fd_touch, EV_SYN, SYN_REPORT, 0);

    // Release the pen input
    ioctl(fd_pen, EVIOCGRAB, 0);

    // Bring the pen back into proximity of the surface.
    writeEventVals(fd_pen, EV_KEY, BTN_TOOL_PEN, 1);
    writeEventVals(fd_pen, EV_SYN, SYN_REPORT, 0);
}

bool laterThan(struct timeval now, struct timeval then, double delta) {
    double elapsed = (now.tv_sec - then.tv_sec) + (now.tv_usec / 1000000.0 - then.tv_usec / 1000000.0);
    return elapsed > delta;
}

void mainloop(int fd_pen, int fd_touch, bool toggle) {
    struct input_event ev_pen;
    const size_t ev_pen_size = sizeof(struct input_event);
    int n_clicks = 0;
    bool primed = false;
    struct timeval last_click;
    bool eraser_on = false;

    for (;;) {
        read(fd_pen, &ev_pen, ev_pen_size); //note: read pauses until there is data

        if (ev_pen.type == EV_KEY && ev_pen.code == BTN_STYLUS) {
            if (!toggle)
                writeEventVals(fd_pen, EV_KEY, BTN_TOOL_RUBBER, ev_pen.value);
            if (ev_pen.value == 1) {  // press
                if (n_clicks != 0 && laterThan(ev_pen.time, last_click, PRESS_TIMEOUT))  // Outstanding event?
                    n_clicks = 0;
                n_clicks += 1;
                last_click = ev_pen.time;
                primed = false;
            } else if (ev_pen.value == 0) {  // release
                if (!laterThan(ev_pen.time, last_click, PRESS_TIMEOUT)) {
                    primed = true;
                    last_click = ev_pen.time;
                } else {
                    n_clicks = 0;
                    primed = false;
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
                writeMultiTap(fd_touch, fd_pen, n_clicks);
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
    int fd_pen, fd_touch;

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
    /* Open Device: Touch */
    fd_touch = open(TOUCH_DEVICE, O_RDWR);
    if (fd_touch == -1) {
        fprintf(stderr, "%s is not a vaild device\n", PEN_DEVICE);
        exit(EXIT_FAILURE);
    }

    /* Print Device Name */
    if (verbose) {
        char name[256] = "Unknown";
        ioctl(fd_pen, EVIOCGNAME(sizeof(name)), name);
        printf("Using Devices:\n");
        printf("1. device file = %s\n", PEN_DEVICE);
        printf("   device name = %s\n", name);
        ioctl(fd_touch, EVIOCGNAME(sizeof(name)), name);
        printf("2. device file = %s\n", TOUCH_DEVICE);
        printf("   device name = %s\n", name);
    }

    mainloop(fd_pen, fd_touch, toggle_mode);
    return EXIT_SUCCESS;
}
