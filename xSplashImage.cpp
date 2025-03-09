
/** ********************************************************
 ** Main xSplashImage. Minimally create and display an x11
 ** window SplashPage from a local XPM image file.
 **/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <malloc.h>
#include <string>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xutil.h>

using namespace std;


/** ********************************************************
 ** Module globals and consts.
 **/
Display* mDisplay;

unsigned int mScreenWidth = 0;
unsigned int mScreenHeight = 0;

XImage* mSplashImage;

Window mSplashWindow;

bool isThisDesktopGnome();
void mergeRootImageUnderSplashImage(XImage* rootImage,
    XImage* splashImage);

void debugXImage(string TAG, XImage* IMAGE);


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
    mScreenWidth = WidthOfScreen(DefaultScreenOfDisplay(mDisplay));
    mScreenHeight = HeightOfScreen(DefaultScreenOfDisplay(mDisplay));

    // Read XPM SplashImage from file.
    XpmAttributes splashAttributes;
    splashAttributes.valuemask = XpmSize;
    XpmReadFileToImage(mDisplay, "xSplashImage.xpm",
        &mSplashImage, NULL, &splashAttributes);

    // Determine where to center Splash.
    const int CENTERED_X_POS =
        (mScreenWidth - splashAttributes.width) / 2;
    const int CENTERED_Y_POS =
        (mScreenHeight - splashAttributes.height) / 2;

    // Get root image under Splash & merge for transparency.
    XImage* rootImage = XGetImage(mDisplay,
        DefaultRootWindow(mDisplay), CENTERED_X_POS,
        CENTERED_Y_POS, splashAttributes.width,
        splashAttributes.height, XAllPlanes(), ZPixmap);
    mergeRootImageUnderSplashImage(rootImage, mSplashImage);

    // Create our X11 Window to host the splash image.
    mSplashWindow = XCreateSimpleWindow(mDisplay,
        DefaultRootWindow(mDisplay), 0, 0,
        splashAttributes.width, splashAttributes.height, 1,
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

    // Gnome && KDE get different X11 event counts that
    // determine when the SplashPage has been completely DRAWN.
    const int EXPOSED_EVENT_COUNT_NEEDED =
        isThisDesktopGnome() ? 1 : 3;

    // Show SplashImage in the window. Consume X11 events.
    // Respond to Expose event for DRAW.
    XSelectInput(mDisplay, mSplashWindow, ExposureMask);
    int exposedEventCount = 0;

    bool finalEventReceived = false;
    while (!finalEventReceived) {
        XEvent event;
        XNextEvent(mDisplay, &event);
        switch (event.type) {
            case Expose:
                if (++exposedEventCount >= EXPOSED_EVENT_COUNT_NEEDED) {
                    XPutImage(mDisplay, mSplashWindow,
                        XCreateGC(mDisplay, mSplashWindow, 0, 0),
                        mSplashImage, 0, 0, 0, 0, splashAttributes.width,
                        splashAttributes.height);
                    finalEventReceived = true;
                }
        }
    }
    XFlush(mDisplay);

    // Sleep with window displayed, then close.
    sleep(5);

    XpmFreeAttributes(&splashAttributes);
    XFree(mSplashImage);
    XFree(rootImage);
    XUnmapWindow(mDisplay, mSplashWindow);
    XDestroyWindow(mDisplay, mSplashWindow);

    XCloseDisplay(mDisplay);
    return false;
}

/** ********************************************************
 ** Helper method to determine if desktop is Gnome or not.
 **/
bool isThisDesktopGnome() {
    char* desktop = getenv("XDG_SESSION_DESKTOP");
    for (char* eachChar = desktop; *eachChar; ++eachChar) {
        *eachChar = tolower(*eachChar);
    }

    return strstr(desktop, "gnome") ||
           strstr(desktop, "ubuntu");
}

/** ********************************************************
 ** Copies root screen image "under" the splashImage.
 ** "Transparent" pixels will reveal the root window.
 **/
void mergeRootImageUnderSplashImage(XImage* rootImage,
    XImage* splashImage) {
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
                    *((rootImage->data) + byteI + 0); // Blue.
                *((splashImage->data) + byteI + 1) =
                    *((rootImage->data) + byteI + 1); // Green.
                *((splashImage->data) + byteI + 2) =
                    *((rootImage->data) + byteI + 2); // Red.
                *((splashImage->data) + byteI + 3) =
                    *((rootImage->data) + byteI + 3);
            }
        }
    }
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
