
/**
 * Minimally create and display an x11 window SplashPage
 * from a locally defined XPM image file.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <malloc.h>
#include <string>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xutil.h>

#include "xSplashImage.h"

using namespace filesystem;

 /**
 * Module globals.
 */
Atom mAtomDMSupportsWMCheck;
Atom mAtomGetWMName;
Atom mAtomGetUTF8String;

Display* mDisplay;
XpmAttributes mSplashImageAttr;
Window mSplashWindow;

/**
 * Module Entry.
 */
int main(int argc, char *argv[]) {

    // Check for display error.
    const char* WAYLAND_DISPLAY = getenv("WAYLAND_DISPLAY");
    if (WAYLAND_DISPLAY && strlen(WAYLAND_DISPLAY) > 0) {
        const char* TEMP = WAYLAND_DISPLAY ?
            WAYLAND_DISPLAY : "";
        cout << COLOR_RED << endl << "xSplashImage: Wayland "
            "Display Manager is detected, FATAL." <<
            COLOR_NORMAL << endl;
        cout << COLOR_YELLOW << "xSplashImage: env var "
            "$WAYLAND_DISPLAY == \"" << TEMP <<
            "\"." << COLOR_NORMAL << endl;
        return true;
    }

    // Check for session error.
    const char* SESSION_TYPE = getenv("XDG_SESSION_TYPE");
    if (strcmp(SESSION_TYPE, "x11") != 0) {
        cout << endl << COLOR_RED << "xSplashImage: No X11 "
            "Session type is detected, FATAL." <<
            COLOR_NORMAL << endl;
        cout << COLOR_YELLOW << "xSplashImage: env var "
            "$XDG_SESSION_TYPE == \"" << SESSION_TYPE <<
            "\"." << COLOR_NORMAL << endl;
        return true;
    }

    // Check for display access.
    mDisplay = XOpenDisplay(NULL);
    if (mDisplay == NULL) {
        cout << COLOR_RED << "\nxSplashImage: X11 Display "
            "Does not seem to be available (Are you Wayland?) "
            "FATAL." << COLOR_NORMAL << endl;
        return true;
    }

    // Check for Window Manager.
    mAtomDMSupportsWMCheck = XInternAtom(mDisplay,
        "_NET_SUPPORTING_WM_CHECK", False);
    mAtomGetWMName = XInternAtom(mDisplay, "_NET_WM_NAME", False);
    mAtomGetUTF8String = XInternAtom(mDisplay, "UTF8_STRING", False);
    const string WM_NAME = getWindowManagerName();

    // Setup x11 Error handler.
    XSetErrorHandler(handleX11ErrorEvent);

    // Read XPM SplashImage from file.
    XImage* splashImage;
    mSplashImageAttr.valuemask = XpmSize;
    XpmReadFileToImage(mDisplay, SPLASH_IMAGE_FILENAME.c_str(),
        &splashImage, NULL, &mSplashImageAttr);

    // Determine where to center SplashImage.
    const int SCREEN_WIDTH = (WidthOfScreen(
        DefaultScreenOfDisplay(mDisplay)));
    const int SCREEN_HEIGHT = (HeightOfScreen(
        DefaultScreenOfDisplay(mDisplay)));
    const int CENTER_X = (SCREEN_WIDTH -
        mSplashImageAttr.width) / 2;
    const int CENTER_Y = (SCREEN_HEIGHT -
        mSplashImageAttr.height) / 2;
    mergeRootImageUnderSplashImage(splashImage,
        CENTER_X, CENTER_Y);

    // Display logging info.
    cout << endl;
    cout << COLOR_BLUE << "Display Manager (DM) : " <<
        getDisplayManagerType() << "." << endl;
    cout << COLOR_BLUE << "Session         (SE) : " <<
        SESSION_TYPE << "." << endl;
    cout << COLOR_BLUE << "Desktop Environ (DE) : " <<
        getenv("XDG_CURRENT_DESKTOP") << "." << endl;
    cout << COLOR_NORMAL;
    cout << COLOR_BLUE << "Window Manager  (WM) : " <<
        WM_NAME << "." << COLOR_NORMAL << endl;

    cout << endl;
    cout << "Screen Size      : " << SCREEN_WIDTH <<
        ", " << SCREEN_HEIGHT << "." << endl;
    cout << "SplashImage size : " << mSplashImageAttr.width <<
        ", " << mSplashImageAttr.height << "." << endl;
    cout << "Centered Pos     : " << CENTER_X <<
        ", " << CENTER_Y << "." << endl << endl;

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
    displaySplashImage(splashImage);

    XDestroyImage(splashImage);
    XpmFreeAttributes(&mSplashImageAttr);
    XUnmapWindow(mDisplay, mSplashWindow);
    XDestroyWindow(mDisplay, mSplashWindow);
    XCloseDisplay(mDisplay);

    return false;
}

/**
 * Copies Desktop background "under" the splashImage.
 * "Transparent" pixels will reveal the desktop image
 * if available, else simply black.
 */
void mergeRootImageUnderSplashImage(XImage* splashImage,
    int xPos, int yPos) {

    XImage* desktopImage = XGetImage(mDisplay,
        DefaultRootWindow(mDisplay), xPos, yPos,
        mSplashImageAttr.width, mSplashImageAttr.height,
        AllPlanes, ZPixmap);
    if (!desktopImage) {
        desktopImage = createBlackXImage();
    }

    for (int h = 0; h < splashImage->height; h++) {
        const int rowI = h * splashImage->width * 4;

        for (int w = 0; w < splashImage->width; w++) {
            const int byteI = rowI + (w * 4);

            const unsigned char fromByte0 =
                *((splashImage->data) + byteI + 0);
            const unsigned char fromByte1 =
                *((splashImage->data) + byteI + 1);
            const unsigned char fromByte2 =
                *((splashImage->data) + byteI + 2);

            // Copy root pixel if this considered transparent.
            if (fromByte0 == 0x30 && fromByte1 == 0x30 &&
                fromByte2 == 0x30) {
                // Blue.
                *((splashImage->data) + byteI + 0) =
                    *((desktopImage->data) + byteI + 0);
                // Green.
                *((splashImage->data) + byteI + 1) =
                    *((desktopImage->data) + byteI + 1);
                // Red.
                *((splashImage->data) + byteI + 2) =
                    *((desktopImage->data) + byteI + 2);
                // Opacity.
                *((splashImage->data) + byteI + 3) =
                    *((desktopImage->data) + byteI + 3);
            }
        }
    }

    XDestroyImage(desktopImage);
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
        free(allocData);
    }
    return resultImage;
}

/**
 * Display the splash screen, pause, then destroy it.
 */
void displaySplashImage(XImage* splashImage) {
    XSelectInput(mDisplay, mSplashWindow, ExposureMask);

    while (true) {
        XEvent event;
        XNextEvent(mDisplay, &event);

        // Process Expose Events.
        if (event.type == Expose) {
            const XExposeEvent* EVENT =
                (XExposeEvent*) &event;
            debugXExposeEvent(EVENT);

            // Respond to Expose event for DRAW.
            if (XPending(mDisplay) == 0 &&
                EVENT->width > 1 && EVENT->height > 1) {
                XPutImage(mDisplay, mSplashWindow, XCreateGC(
                    mDisplay, mSplashWindow, 0, 0), splashImage,
                    0, 0, 0, 0, mSplashImageAttr.width,
                    mSplashImageAttr.height);
                break; // Finished
            }
            continue;
        }

        // Debug all other Events.
        const XAnyEvent* ANY_EVENT = (XAnyEvent*) &event;
        debugXAnyEvent(ANY_EVENT);
    }

    // Sleep with window displayed.
    XFlush(mDisplay);
    sleep(5);
}

/**
 * Helper method to determine the Display Manager (DM).
 */
string getDisplayManagerType() {
    const string UNKNOWN_DM_STRING = "(Unknown)";

    string result = UNKNOWN_DM_STRING;
    const char* CMD = "ps --ppid 1";
    FILE* procsFile = popen(CMD, "r");
    if (!procsFile) {
        return result;
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

    pclose(procsFile);
    return result;
}

/**
 * This method returns the Window Managers name.
 */
string getWindowManagerName() {
    if (!displayCanReportWMName()) {
        cout << endl << COLOR_YELLOW << "DM unable "
            "to report WM Name." << COLOR_NORMAL << endl;
        return {};
    }

    const Window ROOT_WINDOW = getRootWindowFromDisplay();
    if (!ROOT_WINDOW) {
        cout << endl << COLOR_YELLOW << "DM unable "
            "to report Root Window." << COLOR_NORMAL <<
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
bool displayCanReportWMName() {
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
            }
            XFree(resultWindowPtr);
            resultWindowPtr = nullptr;
        }
    }

    return resultWindow;
}

/**
 * Helper method to get the WM Name from the
 * DM's Root Window.
 */
string getWMNameFromRootWindow(Window rootWindow) {
    if (rootWindow == None) {
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
                const std::string RESULT_NAME(reinterpret_cast
                    <char*> (resultWindowPtr), resultCount);
                XFree(resultWindowPtr);
                resultWindowPtr = nullptr;
                return RESULT_NAME;
            }
        }
    }

    return {};
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
        COLOR_RED, msg, COLOR_NORMAL);

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
            const int byteI = rowI + (w * 4);
            const unsigned char byte0 = *((image->data) + byteI + 0);
            const unsigned char byte1 = *((image->data) + byteI + 1);
            const unsigned char byte2 = *((image->data) + byteI + 2);
            const unsigned char byte3 = *((image->data) + byteI + 3);

            printf("[%02x %02x %02x %02x]  ",
                byte0, byte1, byte2, byte3);
        }
        printf("\n");
    }
}

/**
 * Helper method to debug XExposeEvents.
 */
void debugXExposeEvent(const XExposeEvent* event) {
    cout << "XExposeEvent Window: " << event->window <<
        " Display: " << event->display << endl;
    cout << "   Type: " << event->type <<
        " serial: " << event->serial <<
        " send_event: " << event->send_event << endl;
    cout << "    Pos: " << event->x << ", " << event->y <<
        " Size: " << event->width << ", " << event->height <<
        " Count: " << event->count <<
        " XPending: " << XPending(mDisplay) << endl;
    cout << endl;
}

/**
 * Helper method to debug any x11 XEvent.
 */
void debugXAnyEvent(const XAnyEvent* event) {
    cout << "XAnyEvent Window: " << event->window <<
        " Display: " << event->display << endl;
    cout << "   Type: " << event->type <<
        " Serial: " << event->serial <<
        " Send_event: " << event->send_event << endl;
    cout << endl;
}
