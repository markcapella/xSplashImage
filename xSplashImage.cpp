
/** ********************************************************
 ** Main xSplashImage. Minimally create and display an x11
 ** window SplashPage from a local XPM image file.
 **/
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


/** ********************************************************
 ** Module globals and consts.
 **/
DMType mDisplayManagerType;
Display* mDisplay;

XpmAttributes mSplashImageAttributes;
Window mSplashWindow;

/** ********************************************************
 ** Module Entry.
 **/
int main(int argc, char *argv[]) {
    // Gnome && KDE get different X11 event counts that
    // determine when the SplashPage has been completely DRAWN.
    mDisplayManagerType = getDisplayManagerType();
    if (mDisplayManagerType == DMType::INDETERMINATE) {
        fprintf(stderr, "\nxSplashImage: Cannot determine "
            "Display Manager, FATAL.\n");
        return true;
    }

    mDisplay = XOpenDisplay(NULL);
    if (mDisplay == NULL) {
        printf("\nxSplashImage: X11 Display Does not seem "
            "to be available - FATAL. Are you Wayland?\n");
        return true;
    }

    // Setup Error handler.
    XSetErrorHandler(handleX11ErrorEvent);

    // Read XPM SplashImage from file.
    XImage* splashImage;
    mSplashImageAttributes.valuemask = XpmSize;
    XpmReadFileToImage(mDisplay, "xSplashImage.xpm",
        &splashImage, NULL, &mSplashImageAttributes);

    // Determine where to center our splash. Get screen
    // dimensions, & merge desktop background under.
    cout << "WidthOfScreen.width: " <<
        WidthOfScreen(DefaultScreenOfDisplay(mDisplay)) << endl;
    cout << "HeightOfScreen.height: " <<
        HeightOfScreen(DefaultScreenOfDisplay(mDisplay)) << endl;

    cout << "mSplashImageAttributes.width: " <<
        mSplashImageAttributes.width << endl;
    cout << "mSplashImageAttributes.height: " <<
        mSplashImageAttributes.height << endl;

    const int CENTERED_X_POS =
        (WidthOfScreen(DefaultScreenOfDisplay(mDisplay)) -
            mSplashImageAttributes.width) / 2;
    const int CENTERED_Y_POS =
        (HeightOfScreen(DefaultScreenOfDisplay(mDisplay)) -
            mSplashImageAttributes.height) / 2;

    cout << "CENTERED_X_POS: " << CENTERED_X_POS << endl;
    cout << "CENTERED_Y_POS: " << CENTERED_Y_POS << endl;

    mergeRootImageUnderSplashImage(splashImage,
        CENTERED_X_POS, CENTERED_Y_POS);

    // Create our X11 mSplashWindow to host the image.
    mSplashWindow = XCreateSimpleWindow(mDisplay,
        DefaultRootWindow(mDisplay), 0, 0,
        mSplashImageAttributes.width, mSplashImageAttributes.height, 1,
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
        CENTERED_X_POS, CENTERED_Y_POS);

    displaySplash(splashImage);
    XDestroyImage(splashImage);

    XpmFreeAttributes(&mSplashImageAttributes);

    XUnmapWindow(mDisplay, mSplashWindow);
    XDestroyWindow(mDisplay, mSplashWindow);

    XCloseDisplay(mDisplay);
    return false;
}

/** ********************************************************
 ** Copies root screen image "under" the splashImage.
 ** "Transparent" pixels will reveal the root window
 ** if available, else simply black.
 **/
void mergeRootImageUnderSplashImage(XImage* splashImage,
    int xPos, int yPos) {

    // Try to use desktop image for Splash
    // transparency, else, just use black.
    XImage* desktopImage = XGetImage(mDisplay, DefaultRootWindow(
        mDisplay), xPos, yPos, mSplashImageAttributes.width,
        mSplashImageAttributes.height, AllPlanes, ZPixmap);
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
                *((splashImage->data) + byteI + 0) =
                    *((desktopImage->data) + byteI + 0); // Blue.
                *((splashImage->data) + byteI + 1) =
                    *((desktopImage->data) + byteI + 1); // Green.
                *((splashImage->data) + byteI + 2) =
                    *((desktopImage->data) + byteI + 2); // Red.
                *((splashImage->data) + byteI + 3) =
                    *((desktopImage->data) + byteI + 3);
            }
        }
    }

    XDestroyImage(desktopImage);
}

/** ********************************************************
 ** Helper method ...
 **/
XImage* createBlackXImage() {
    char* allocData = (char*)
        calloc(mSplashImageAttributes.width *
            mSplashImageAttributes.height * 4, 1);

    XImage* resultImage = XCreateImage(mDisplay,
        DefaultVisual(mDisplay, DefaultScreen(mDisplay)),
        DefaultDepth(mDisplay, DefaultScreen(mDisplay)),
        ZPixmap, 0, allocData,
        mSplashImageAttributes.width,
        mSplashImageAttributes.height,
        32, 0);

    if (!resultImage) {
        free(allocData);
    }
    return resultImage;
}

/** ********************************************************
 ** Helper method to determine if the display manager
 ** is SDDM or GDM. Gnome && KDE get different X11 event
 ** counts that determine when the SplashPage has been
 ** completely DRAWN.
 **/
DMType getDisplayManagerType() {
    const string FILE1 = "/etc/X11/default-display-manager";
    const DMType FILE1_DMTYPE = getDMTypeFromFile(FILE1);
    if (FILE1_DMTYPE != DMType::INDETERMINATE) {
        return FILE1_DMTYPE;
    }

    const string FILE2 = "/etc/sysconfig/desktop";
    const DMType FILE2_DMTYPE = getDMTypeFromFile(FILE2);
    if (FILE2_DMTYPE != DMType::INDETERMINATE) {
        return FILE2_DMTYPE;
    }

    const string FILE3 = "/etc/sysconfig/displaymanager";
    const DMType FILE3_DMTYPE = getDMTypeFromFile(FILE3);
    if (FILE3_DMTYPE != DMType::INDETERMINATE) {
        return FILE3_DMTYPE;
    }

    return DMType::INDETERMINATE;
}

/** ********************************************************
 ** Helper method to determine if the Display Manager
 ** identity Type is found in a file.
 **/
DMType getDMTypeFromFile(const string inFile) {
    if (!exists(inFile)) {
        return DMType::INDETERMINATE;
    }

    if (is_directory(status(inFile))) {
        return DMType::INDETERMINATE;
    }

    fstream inStream;
    inStream.open(canonical(inFile), ios::in);
    if (!inStream.is_open()) {
        return DMType::INDETERMINATE;
    }

    string inString;
    while (getline(inStream, inString)) {
        cout << "file: " << inFile << " = " << inString << "." << endl;
        if (inString.find("sddm")) {
            inStream.close();
            cout << "   specifies: sddm as DM." << endl;
            return DMType::SDDM;
        }
        if (inString.find("gdm")) {
            inStream.close();
            cout << "   specifies: gdm as DM." << endl;
            return DMType::GDM;
        }
    }

    inStream.close();
    return DMType::INDETERMINATE;
}

/** ********************************************************
 ** Display the splash screen, pause, then destroy it.
 **/
void displaySplash(XImage* splashScreen) {
    const int EXPOSED_EVENT_COUNT_NEEDED =
        mDisplayManagerType == DMType::GDM ? 3 : 1;

    cout << "EXPOSED_EVENT_COUNT_NEEDED: " <<
        EXPOSED_EVENT_COUNT_NEEDED << endl;

    // Show SplashImage in the window. Consume X11 events.
    // Respond to Expose event for DRAW.
    XSelectInput(mDisplay, mSplashWindow, ExposureMask);

    int exposedEventCount = 0;
    bool finalEventReceived = false;

    while (!finalEventReceived) {
        XEvent event;
        XNextEvent(mDisplay, &event);
        debugXEvent(event);

        switch (event.type) {
            case Expose:
                if (++exposedEventCount >= EXPOSED_EVENT_COUNT_NEEDED) {
                    XPutImage(mDisplay, mSplashWindow,
                        XCreateGC(mDisplay, mSplashWindow, 0, 0),
                        splashScreen, 0, 0, 0, 0, mSplashImageAttributes.width,
                        mSplashImageAttributes.height);
                    finalEventReceived = true;
                }
        }
    }
    XFlush(mDisplay);

    // Sleep with window displayed.
    sleep(5);
}

/** ************************************************
 ** This method traps and handles X11 errors.
 **
 ** Primarily, we close the app if the system doesn't seem sane.
 **/
int handleX11ErrorEvent(Display* dpy, XErrorEvent* event) {
    // Save error & quit early if simply BadWindow/BadMatch.
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

/** ********************************************************
 ** Helper method to debug the XImage contents.
 **/
void debugXImage(string tag, XImage* image) {
    printf("\ndebugXImage() width / height : %d x %d\n",
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

/** ********************************************************
 ** Helper method to debug the XImage contents.
 **/
void debugXEvent(XEvent event) {
    const XAnyEvent* ANY_EVENT = (XAnyEvent*) &event;

    cout << "X11Event Window: " << ANY_EVENT->window <<
        " Display: " << ANY_EVENT->display <<
        " Type: " << ANY_EVENT->type << endl;
}
