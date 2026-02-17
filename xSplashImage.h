
#pragma once

/**
 * Minimally create and display an x11 window SplashPage
 * from a locally defined XPM image file.
 */
#include <string>

using namespace std;

/**
 * Module Type & Defines.
 */
enum class DMType {
    INDETERMINATE,
    GDM,
    SDDM
};

#define COLOR_NORMAL "\033[0m"
#define COLOR_BLACK "\033[0;30m"
#define COLOR_WHITE "\033[0;37m"
#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN "\033[1;36m"

/**
 * Module Consts.
 */
const string SPLASH_IMAGE_FILENAME =
    "xSplashImage.xpm";

/**
 * Module Method definitions.
 */
void mergeRootImageUnderSplashImage(
    XImage* splashImage, int xPos, int yPos);
XImage* createBlackXImage();

void displaySplashImage(XImage* splashImage);

string getDisplayManagerType();
string getWindowManagerName();

bool displayCanReportWMName();
Window getRootWindowFromDisplay();
string getWMNameFromRootWindow(Window rootWindow);

int handleX11ErrorEvent(Display* mDisplay,
    XErrorEvent* event);

void debugXImage(string tag, XImage* image);
void debugXAnyEvent(const XAnyEvent* event);
void debugXExposeEvent(const XExposeEvent* event);
