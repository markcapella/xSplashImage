
/** ********************************************************
 ** Main xSplashImage. Minimally create and display an x11
 ** window SplashPage from a local XPM image file.
 **/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <malloc.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xutil.h>


/** ********************************************************
 ** Module globals and consts.
 **/
Display* mDisplay;

Screen* mScreen;
unsigned int mScreenWidth = 0;
unsigned int mScreenHeight = 0;

Window mSplashWindow;
XImage* mSplashImage;
XpmAttributes mSplashAttributes;

bool mSplashWindowMapped = false;
int mSplashWindowExposedCount = 0;


/** ********************************************************
 ** Memory usage helper method.
 **/
void displayCurrentMemory() {
    struct mallinfo2 startMI2 = mallinfo2();
    printf("\nxSplashImage: Total arena bytes      : %li.\n",
        startMI2.arena);
    printf("xSplashImage: Total allocated bytes  : %li.\n",
        startMI2.uordblks);
    printf("xSplashImage: Total free bytes       : %li.\n",
        startMI2.fordblks);
}

/** ********************************************************
 ** Module Entry.
 **/
int main(int argc, char *argv[]) {
    mDisplay = XOpenDisplay(NULL);
    if (mDisplay == NULL) {
        printf("\nxSplashImage: X11 Display Does not seem to be "
            "available - FATAL. Are you Wayland?\n");
        return true;
    }

    // Display initial memory useage to log.
    mScreen = DefaultScreenOfDisplay(mDisplay);
    mScreenWidth = WidthOfScreen(mScreen);
    mScreenHeight = HeightOfScreen(mScreen);

    displayCurrentMemory();

    // Read XPM SplashImage from file.
    mSplashAttributes.valuemask = 0;
    mSplashAttributes.bitmap_format = ZPixmap;
    mSplashAttributes.valuemask |= XpmBitmapFormat;
    mSplashAttributes.valuemask |= XpmSize;
    mSplashAttributes.valuemask |= XpmCharsPerPixel;
    mSplashAttributes.valuemask |= XpmReturnPixels;
    mSplashAttributes.valuemask |= XpmReturnAllocPixels;
    mSplashAttributes.exactColors = False;
    mSplashAttributes.valuemask |= XpmExactColors;
    mSplashAttributes.closeness = 30000;
    mSplashAttributes.valuemask |= XpmCloseness;
    mSplashAttributes.alloc_close_colors = False;
    mSplashAttributes.valuemask |= XpmAllocCloseColors;

    XpmReadFileToImage(mDisplay, "xSplashImage.xpm",
        &mSplashImage, NULL, &mSplashAttributes);

    // Create our X11 Window to host the image.
    mSplashWindow = XCreateSimpleWindow(mDisplay,
        DefaultRootWindow(mDisplay), 0, 0, mSplashAttributes.width,
        mSplashAttributes.height, 1, BlackPixel(mDisplay, 0),
        WhitePixel(mDisplay, 0));

    // Set window to dock (no titlebar or close button).
    const Atom WINDOW_TYPE = XInternAtom(mDisplay,
        "_NET_WM_WINDOW_TYPE", False);
    const long TYPE_VALUE = XInternAtom(mDisplay,
        "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(mDisplay, mSplashWindow, WINDOW_TYPE, XA_ATOM,
        32, PropModeReplace, (unsigned char*) &TYPE_VALUE, 1);

    // Add the image to the host window.
    XPutImage(mDisplay, mSplashWindow, XCreateGC(mDisplay,
        mSplashWindow, 0, 0), mSplashImage, 0, 0, 0, 0,
        mSplashAttributes.width, mSplashAttributes.height);

    // Show SplashImage in the window.
    XSelectInput(mDisplay, mSplashWindow, ExposureMask);
    Atom mDeleteMessage = XInternAtom(mDisplay,
        "WM_DELETE_WINDOW", False);
    XSetWMProtocols(mDisplay, mSplashWindow,
        &mDeleteMessage, 1);

    // Gnome && KDE git different X11 event counts that
    // determine when the SplashPage has finished loading.
    char* desktop = getenv("XDG_SESSION_DESKTOP");
    for (char* eachChar = desktop; *eachChar; ++eachChar) {
        *eachChar = tolower(*eachChar);
    }
    const bool IS_THIS_KDE = !strstr(desktop, "gnome") &&
        !strstr(desktop, "ubuntu");

    const int FINAL_EXPOSE_EVENT_COUNT =
        IS_THIS_KDE ? 3 : 1;
    printf("\nxSplashImage: For %s we're expecting %i "
        "expose event%s.\n", IS_THIS_KDE ? "KDE" : "GNOME/UBUNTU",
        FINAL_EXPOSE_EVENT_COUNT,
        FINAL_EXPOSE_EVENT_COUNT > 1 ? "s" : "");

    bool finalEventReceived = false;
    while (!finalEventReceived) {
        if (!mSplashWindowMapped) {
            printf("xSplashImage: Performing initial mapping.\n\n");
            XMapWindow(mDisplay, mSplashWindow);
            // Gnome needs to be centered. KDE does
            // it for you.
            if (!IS_THIS_KDE) {
                const int CENTERED_X_POS =
                    (mScreenWidth - mSplashAttributes.width) / 2;
                const int CENTERED_Y_POS =
                    (mScreenHeight - mSplashAttributes.height) / 2;
                XMoveWindow(mDisplay, mSplashWindow,
                    CENTERED_X_POS, CENTERED_Y_POS);
            }
            mSplashWindowMapped = true;
        }

        XEvent event;
        XNextEvent(mDisplay, &event);
        switch (event.type) {
            case ClientMessage:
                if (event.xclient.data.l[0] ==
                        (long int) mDeleteMessage) {
                    finalEventReceived = true;
                }
                break;

            case Expose:
                if (event.xexpose.window != mSplashWindow) {
                    printf("xSplashImage: 3d Expose event "
                        "WRONG WINDOW.\n");
                    break;
                }

                mSplashWindowExposedCount++;
                printf("xSplashImage: Expose Event %i received.\n",
                    mSplashWindowExposedCount);

                XPutImage(mDisplay, mSplashWindow,
                    XCreateGC(mDisplay, mSplashWindow, 0, 0),
                    mSplashImage, 0, 0, 0, 0, mSplashAttributes.width,
                    mSplashAttributes.height);

                if (mSplashWindowExposedCount >= FINAL_EXPOSE_EVENT_COUNT) {
                    printf("xSplashImage: 3d Expose event signals END.\n");
                    finalEventReceived = true;
                }
                break;
        }
    }

    // Pause for user to admire SplashImage.
    sleep(5);

    // Close & destroy window, display.
    XUnmapWindow(mDisplay, mSplashWindow);
    XDestroyWindow(mDisplay, mSplashWindow);
    XCloseDisplay(mDisplay);

    // Display final memory useage to log.
    displayCurrentMemory();
    return false;
}