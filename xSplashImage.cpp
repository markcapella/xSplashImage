
/**
 * Minimally create and display an x11 window SplashPage
 * from a locally defined XPM image file.
 */
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <fstream>
#include <iostream>
#include <malloc.h>
#include <ncurses.h>
#include <string>
#include <thread>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xutil.h>

#include "xSplashImage.h"


/**
 * Module Consts.
 */
Atom mAtomDMSupportsWMCheck;
Atom mAtomGetWMName;
Atom mAtomGetUTF8String;

Display* mDisplay;
XpmAttributes mSplashImageAttr;
XImage* mSplashImage;
Window mSplashWindow;

/**
 * Module Entry.
 */
int main(int argc, char* argv[]) {
    // Check for input file.
    if (argc < 2) {
        cout << XCOLOR_RED << endl << "xSplashImage: No " <<
            "input XImage named on command line, FATAL." <<
            XCOLOR_NORMAL << endl;
        return true;
    }
    const char* SPLASH_IMAGE_FILENAME = argv[1];

    // Check for display error.
    const char* WAYLAND_DISPLAY = getenv("WAYLAND_DISPLAY");
    if (WAYLAND_DISPLAY && strlen(WAYLAND_DISPLAY) > 0) {
        const char* TEMP = WAYLAND_DISPLAY ?
            WAYLAND_DISPLAY : "";
        cout << XCOLOR_RED << endl << "xSplashImage: Wayland "
            "Display Manager is detected, FATAL." <<
            XCOLOR_NORMAL << endl;
        cout << XCOLOR_YELLOW << "xSplashImage: env var "
            "$WAYLAND_DISPLAY: \"" << TEMP <<
            "\"." << XCOLOR_NORMAL << endl;
        return true;
    }

    // Check for session error.
    const char* SESSION_TYPE = getenv("XDG_SESSION_TYPE");
    if (strcmp(SESSION_TYPE, "x11") != 0) {
        cout << endl << XCOLOR_RED << "xSplashImage: No X11 "
            "Session type is detected, FATAL." <<
            XCOLOR_NORMAL << endl;
        cout << XCOLOR_YELLOW << "xSplashImage: env var "
            "$XDG_SESSION_TYPE: \"" << SESSION_TYPE <<
            "\"." << XCOLOR_NORMAL << endl;
        return true;
    }

    // Check for display access.
    mDisplay = XOpenDisplay(NULL);
    if (mDisplay == NULL) {
        cout << XCOLOR_RED << "\nxSplashImage: X11 Display "
            "does not seem to be available (Are you Wayland?) "
            "FATAL." << XCOLOR_NORMAL << endl;
        return true;
    }

    // Check for Window Manager.
    mAtomDMSupportsWMCheck = XInternAtom(mDisplay,
        "_NET_SUPPORTING_WM_CHECK", False);
    mAtomGetWMName = XInternAtom(mDisplay, "_NET_WM_NAME", False);
    mAtomGetUTF8String = XInternAtom(mDisplay, "UTF8_STRING", False);
    const string WM_NAME = getWindowManagerName();

    // Setup x11 Error handler & read XPM SplashImage.
    XSetErrorHandler(handleX11ErrorEvent);
    mSplashImageAttr.valuemask = XpmSize;
    if (XpmReadFileToImage(mDisplay, SPLASH_IMAGE_FILENAME,
        &mSplashImage, NULL, &mSplashImageAttr) == XpmOpenFailed) {
        cout << XCOLOR_RED << "\nxSplashImage: Input file invalid "
            "or non-existant, FATAL." << XCOLOR_NORMAL << endl;
        XCloseDisplay(mDisplay);
        return true;
    }

    // Determine where to center SplashImage.
    const int SCREEN_WIDTH = (WidthOfScreen(
        DefaultScreenOfDisplay(mDisplay)));
    const int SCREEN_HEIGHT = (HeightOfScreen(
        DefaultScreenOfDisplay(mDisplay)));
    const int CENTER_X = (SCREEN_WIDTH -
        mSplashImageAttr.width) / 2;
    const int CENTER_Y = (SCREEN_HEIGHT -
        mSplashImageAttr.height) / 2;
    if (!mergeRootImageUnderSplashImage(CENTER_X, CENTER_Y)) {
        cout << XCOLOR_RED << "\nxSplashImage: Can\'t "
            "create merged Splash Image, FATAL." <<
            XCOLOR_NORMAL << endl;
        XCloseDisplay(mDisplay);
        return true;
    };

    // Display logging info.
    cout << endl;
    cout << XCOLOR_BLUE << "Display Manager (DM) : " <<
        getDisplayManagerName() << "." << endl;
    cout << "Session         (SE) : " <<
        SESSION_TYPE << "." << endl;
    cout << "Desktop Environ (DE) : " <<
        getenv("XDG_CURRENT_DESKTOP") << "." << endl;
    cout << "Window Manager  (WM) : " <<
        WM_NAME << "." << endl;
    cout << XCOLOR_NORMAL << endl;

    cout << "Screen Size      : " << SCREEN_WIDTH <<
        ", " << SCREEN_HEIGHT << "." << endl;
    cout << "SplashImage size : " << mSplashImageAttr.width <<
        ", " << mSplashImageAttr.height << "." << endl;
    cout << "Centered Pos     : " << CENTER_X <<
        ", " << CENTER_Y << "." << endl;
    cout << endl;

    // Create our X11 mSplashWindow to host the image.
    mSplashWindow = XCreateSimpleWindow(mDisplay,
        DefaultRootWindow(mDisplay), 0, 0,
        mSplashImageAttr.width, mSplashImageAttr.height, 1,
        BlackPixel(mDisplay, 0), WhitePixel(mDisplay, 0));

    // Set window to dock (no titlebar or close button).
    const Atom WINDOW_TYPE = XInternAtom(mDisplay,
        "_NET_WM_WINDOW_TYPE", False);
    const long TYPE_VALUE = XInternAtom(mDisplay,
        "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(mDisplay, mSplashWindow, WINDOW_TYPE, XA_ATOM,
        32, PropModeReplace, (unsigned char*) &TYPE_VALUE, 1);

    // Map, then position window for Gnome.
    XMapWindow(mDisplay, mSplashWindow);
    XMoveWindow(mDisplay, mSplashWindow,
        CENTER_X, CENTER_Y);

    // Init NCurses;
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, true);

    // Display.
    displaySplashImage();

    // Uninit NCurses.
    endwin();

    // All other uninit.
    XUnmapWindow(mDisplay, mSplashWindow);
    XDestroyWindow(mDisplay, mSplashWindow);
    XDestroyImage(mSplashImage);
    XpmFreeAttributes(&mSplashImageAttr);
    XCloseDisplay(mDisplay);

    return false;
}

/**
 * This method returns the Window Managers name.
 */
string getWindowManagerName() {
    if (!canDisplayReportWMName()) {
        cout << endl << XCOLOR_YELLOW << "DM unable "
            "to report WM Name." << XCOLOR_NORMAL << endl;
        return {};
    }

    const Window ROOT_WINDOW = getRootWindowFromDisplay();
    if (!ROOT_WINDOW) {
        cout << endl << XCOLOR_YELLOW << "DM unable "
            "to report Root Window." << XCOLOR_NORMAL <<
            endl;
        return {};
    }

    const string WM_NAME = getWMNameFromRootWindow(
        ROOT_WINDOW);
    return WM_NAME == "GNOME Shell" ?
        "Gnome Shell (Mutter)" : WM_NAME;
}

/**
 * Helper method to check if the DM can report
 * information about the WM.
 */
bool canDisplayReportWMName() {
    return mAtomDMSupportsWMCheck && mAtomGetWMName &&
        mAtomGetUTF8String;
}

/**
 * Helper method to get the Root Window of the DM.
 */
Window getRootWindowFromDisplay() {
    Window resultWindow = None;

    Atom resultType;
    int resultFormat;
    unsigned long resultCount;
    unsigned long unused;

    unsigned char* resultWindowPtr = nullptr;
    if (XGetWindowProperty(mDisplay, DefaultRootWindow(mDisplay),
        mAtomDMSupportsWMCheck, 0, 1, False, XA_WINDOW,
        &resultType, &resultFormat, &resultCount,
        &unused, &resultWindowPtr) == Success) {

        if (resultWindowPtr != nullptr) {
            if (resultType == XA_WINDOW &&
                resultFormat == 32 && resultCount == 1) {
                resultWindow = *reinterpret_cast<Window*>
                    (resultWindowPtr);
                XFree(resultWindowPtr);
                resultWindowPtr = nullptr;
                return resultWindow;
            }
            XFree(resultWindowPtr);
            resultWindowPtr = nullptr;
        }
    }

    cout << XCOLOR_YELLOW << "\nxSplashImage: Can\'t get "
       "SupportsWMCheck property root window." <<
        XCOLOR_NORMAL << endl;
    return resultWindow;
}

/**
 * Helper method to get the WM Name from the
 * DM's Root Window.
 */
string getWMNameFromRootWindow(Window rootWindow) {
    if (rootWindow == None) {
        cout << XCOLOR_YELLOW << "\nxSplashImage: Can\'t get "
           "WM name from missing root window." <<
            XCOLOR_NORMAL << endl;
        return {};
    }

    Atom resultType;
    int resultFormat;
    unsigned long resultCount;
    unsigned long unused;

    unsigned char* resultWindowPtr = nullptr;
    if (XGetWindowProperty(mDisplay, rootWindow,
        mAtomGetWMName, 0, 1024, False, mAtomGetUTF8String,
        &resultType, &resultFormat, &resultCount,
        &unused, &resultWindowPtr) == Success) {

        if (resultWindowPtr != nullptr) {
            if (resultType == mAtomGetUTF8String ||
                resultType == XA_STRING) {
                const string RESULT_NAME(reinterpret_cast
                    <char*> (resultWindowPtr), resultCount);
                XFree(resultWindowPtr);
                resultWindowPtr = nullptr;
                return RESULT_NAME;
            }
        }
    }

    cout << XCOLOR_YELLOW << "\nxSplashImage: Can\'t get "
       "GetWMName property from root window." <<
        XCOLOR_NORMAL << endl;
    return {};
}
/**
 * Helper method to determine the Display Manager (DM).
 */
string getDisplayManagerName() {
    const string UNKNOWN_DM_STRING = "(Unknown)";

    const char* CMD = "ps --ppid 1";
    FILE* procsFile = popen(CMD, "r");
    if (!procsFile) {
        cout << XCOLOR_YELLOW << "\nxSplashImage: Can\'t get "
           "running procs list to determine DM Name." <<
            XCOLOR_NORMAL << endl;
        return UNKNOWN_DM_STRING;
    }

    #define BUFFER_LENGTH 1024
    char inFileBuffer[BUFFER_LENGTH];
    try {
        while (fgets(inFileBuffer, BUFFER_LENGTH,
            procsFile) != nullptr) {
            if (strstr(inFileBuffer, "lightdm") != 0) {
                pclose(procsFile);
                return "LightDM";
            }
            if (strstr(inFileBuffer, "sddm") != 0) {
                pclose(procsFile);
                return "Sddm";
            }
            if (strstr(inFileBuffer, "gdm") != 0) {
                pclose(procsFile);
                return "Gdm";
            }
        }
    } catch (...) { }

    cout << XCOLOR_YELLOW << "\nxSplashImage: Can\'t find "
       "DM name in running procs list." <<
        XCOLOR_NORMAL << endl;
    pclose(procsFile);
    return UNKNOWN_DM_STRING;
}

/**
 * Copies Desktop background "under" the splash image.
 * "Transparent" pixels will reveal the desktop image
 * if available, else simply black.
 */
bool mergeRootImageUnderSplashImage(int xPos, int yPos) {
    XImage* desktopImage = XGetImage(mDisplay,
        DefaultRootWindow(mDisplay), xPos, yPos,
        mSplashImageAttr.width, mSplashImageAttr.height,
        AllPlanes, ZPixmap);
    if (!desktopImage) {
        cout << XCOLOR_YELLOW << "\nxSplashImage: Can\'t "
            "get root Desktop image, blending Splash Image "
            "onto black background." << XCOLOR_NORMAL << endl;
        desktopImage = createBlackXImage();
        if (!desktopImage) {
            cout << XCOLOR_RED << "\nxSplashImage: Can\'t "
                "create black background, this is FATAL." <<
                XCOLOR_NORMAL << endl;
            return false;
        }
    }

    for (int h = 0; h < mSplashImage->height; h++) {
        const int rowI = h * mSplashImage->width * 4;

        for (int w = 0; w < mSplashImage->width; w++) {
            const int colI = rowI + (w * 4);

            const unsigned char fromByte0 =
                *((mSplashImage->data) + colI + 0);
            const unsigned char fromByte1 =
                *((mSplashImage->data) + colI + 1);
            const unsigned char fromByte2 =
                *((mSplashImage->data) + colI + 2);

            // Copy root pixel if this considered transparent.
            if (fromByte0 == 0x00 && fromByte1 == 0x00 &&
                fromByte2 == 0x00) {
                // Blue.
                *((mSplashImage->data) + colI + 0) =
                    *((desktopImage->data) + colI + 0);
                // Green.
                *((mSplashImage->data) + colI + 1) =
                    *((desktopImage->data) + colI + 1);
                // Red.
                *((mSplashImage->data) + colI + 2) =
                    *((desktopImage->data) + colI + 2);
                // Opacity.
                *((mSplashImage->data) + colI + 3) =
                    *((desktopImage->data) + colI + 3);
            }
        }
    }

    XDestroyImage(desktopImage);
    return true;
}

/**
 * Helper method to provide a black XImage of
 * desktop size.
 */
XImage* createBlackXImage() {
    char* allocData = (char*)
        calloc(mSplashImageAttr.width *
            mSplashImageAttr.height * 4, 1);

    XImage* resultImage = XCreateImage(mDisplay,
        DefaultVisual(mDisplay, DefaultScreen(mDisplay)),
        DefaultDepth(mDisplay, DefaultScreen(mDisplay)),
        ZPixmap, 0, allocData,
        mSplashImageAttr.width,
        mSplashImageAttr.height,
        32, 0);

    if (!resultImage) {
        cout << XCOLOR_YELLOW << "\nxSplashImage: Can\'t "
            "create black background, this s/b FATAL." <<
            XCOLOR_NORMAL << endl;
        free(allocData);
    }
    return resultImage;
}

/**
 * Display the splash screen, pause, then destroy it.
 */
void displaySplashImage() {
    const chrono::time_point<Clock>
        START_TIME = Clock::now();

    XSelectInput(mDisplay, mSplashWindow,
        ExposureMask | KeyPressMask);

    bool finalExposeEventReceived = false;
    while (!finalExposeEventReceived &&
        !hasUserCancelledSplash()) {

        while (XPending(mDisplay) > 0) {
            XEvent event; XNextEvent(mDisplay, &event);

            // Process Expose Events.
            if (event.type == Expose) {
                const XExposeEvent* EVENT = (XExposeEvent*) &event;
                debugXExposeEvent(EVENT);

                // Respond to Expose event for DRAW.
                if (XPending(mDisplay) == 0 &&
                    EVENT->width > 1 && EVENT->height > 1) {
                    XPutImage(mDisplay, mSplashWindow, XCreateGC(
                        mDisplay, mSplashWindow, 0, 0), mSplashImage,
                        0, 0, 0, 0, mSplashImageAttr.width,
                        mSplashImageAttr.height);
                    finalExposeEventReceived = true;
                }
                continue;
            }

            // Debug all other Events.
            const XAnyEvent* ANY_EVENT = (XAnyEvent*) &event;
            debugXAnyEvent(ANY_EVENT);
        }

    }
    XFlush(mDisplay);

    // If succesful SplashImage (not escaped by keyboard)
    // sleep until rest of time limit.
    if (finalExposeEventReceived) {
        const Milliseconds TIME_MAX(5000);

        const chrono::time_point<Clock> END_TIME = Clock::now();
        const Milliseconds TIME_USED = END_TIME - START_TIME;
        const Milliseconds TIME_REMAIN = TIME_MAX - TIME_USED;
        if (TIME_REMAIN > (Milliseconds) 0) {
            sleepWithTimeoutOrKbdPressed(TIME_REMAIN);
        }
    }
}

/**
 * Helper method to return keyboard state.
 */
bool hasUserCancelledSplash() {
    return getch() != ERR ||
        isMouseClickedAndHeldInWindow(mSplashWindow);
}

/**
 * This method decides if the user is
 * clicking a window.
 */
bool isMouseClickedAndHeldInWindow(Window window) {
    //  Find the focused window pointer click state.
    // If click-state is clicked-down, we're dragging.
    Window root_return, child_return;
    int root_x_return, root_y_return;
    int xPosResult, yPosResult;

    unsigned int pointerState;
    if (XQueryPointer(mDisplay, window,
        &root_return, &child_return, &root_x_return, &root_y_return,
        &xPosResult, &yPosResult, &pointerState)) {
        const unsigned int POINTER_CLICKDOWN = 256;
        return (pointerState & POINTER_CLICKDOWN);
    }

    return false;
}

/**
 * Helper method to sleep up to maximum time or
 * until a key is pressed.
 */
void sleepWithTimeoutOrKbdPressed(Milliseconds
    timeoutValue) {
    const chrono::time_point<Clock>
        START_TIME = Clock::now();

    while (!hasUserCancelledSplash()) {
        const chrono::time_point<Clock> END_TIME = Clock::now();
        const Milliseconds TIME_DIFF = END_TIME - START_TIME;
        if (TIME_DIFF > timeoutValue) {
            break;
        }
        const unsigned POLL_TIME = 50;
        this_thread::sleep_for((Milliseconds) POLL_TIME);
    }
}

/**
 * This method traps and handles X11 errors.
 */
int handleX11ErrorEvent(Display* dpy, XErrorEvent* event) {
    // Continue if simply BadWindow/BadMatch.
    if (event->error_code == BadWindow ||
        event->error_code == BadMatch) {
        return 0;
    }

    // Print the error message of the event.
    const int MAX_MESSAGE_BUFFER_LENGTH = 512;
    char msg[MAX_MESSAGE_BUFFER_LENGTH];
    XGetErrorText(dpy, event->error_code, msg, sizeof(msg));
    printf("%sxSplashImage::handleX11ErrorEvent() %s.%s\n",
        XCOLOR_RED, msg, XCOLOR_NORMAL);

    return 0;
}

/**
 * Helper method to debug the XImage contents.
 */
void debugXImage(string tag, XImage* image) {
    printf("\ndebugXImage() width, height : %d, %d\n",
        image->width, image->height);
    printf("debugXImage() xoffset, format : %d, %d\n\n",
        image->xoffset, image->format);

    for (int h = 0; h < image->height; h++) {
        const int rowI = h * image->width * 4;

        printf("debugXImage() %s : ", tag.c_str());
        for (int w = 0; w < image->width; w++) {
            const int colI = rowI + (w * 4);
            const unsigned char byte0 = *((image->data) + colI + 0);
            const unsigned char byte1 = *((image->data) + colI + 1);
            const unsigned char byte2 = *((image->data) + colI + 2);
            const unsigned char byte3 = *((image->data) + colI + 3);

            printf("[%02x %02x %02x %02x]  ",
                byte0, byte1, byte2, byte3);
        }
        printf("\n");
    }
}

/**
 * Helper method to debug XExposeEvents (Formatted for
 * output in a temp NCurses alternate terminal.
 */
void debugXExposeEvent(const XExposeEvent* event) {
    cout << "XExposeEvent Window: " << event->window <<
        " Display: " << event->display << '\r' << endl;
    cout << "   Type: " << event->type <<
        " serial: " << event->serial <<
        " send_event: " << event->send_event << '\r' << endl;
    cout << "    Pos: " << event->x << ", " << event->y <<
        " Size: " << event->width << ", " << event->height <<
        " Count: " << event->count <<
        " XPending: " << XPending(mDisplay) << '\r' << endl;
    cout << '\r' << endl;
}

/**
 * Helper method to debug XAnyEvent (Formatted for
 * output in a temp NCurses alternate terminal.
 */
void debugXAnyEvent(const XAnyEvent* event) {
    cout << "XAnyEvent Window: " << event->window <<
        " Display: " << event->display << '\r' << endl;
    cout << "   Type: " << event->type <<
        " Serial: " << event->serial <<
        " Send_event: " << event->send_event << '\r' << endl;
    cout << '\r' << endl;
}
