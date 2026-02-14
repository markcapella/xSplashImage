
#pragma once

#include <string>

using namespace std;


/** ********************************************************
 ** Module Type & Defines.
 **/
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

/** ********************************************************
 ** Module Method definitions.
 **/
void mergeRootImageUnderSplashImage(XImage* splashImage,
    int xPos, int yPos);
XImage* createBlackXImage();

DMType getDisplayManagerType();
DMType getDMTypeFromFile(const string inFile);

void displaySplash(XImage* splashScreen);

int handleX11ErrorEvent(Display* dpy, XErrorEvent* event);

void debugXImage(string tag, XImage* image);
void debugXEvent(XEvent event);
