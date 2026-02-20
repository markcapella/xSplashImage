
#pragma once

/**
 * Minimally create and display an x11 window SplashPage
 * from a locally defined XPM image file.
 */
#include <string>

using namespace std;

/**
 * Module Types, Enums, & Defines.
 */
typedef chrono::steady_clock Clock;
typedef chrono::duration<double, std::milli>
    Milliseconds;

enum class DMType {
    INDETERMINATE,
    GDM,
    SDDM
};

#define XCOLOR_NORMAL "\033[0m"
#define XCOLOR_BLACK "\033[0;30m"
#define XCOLOR_WHITE "\033[0;37m"
#define XCOLOR_RED "\033[0;31m"
#define XCOLOR_GREEN "\033[1;32m"
#define XCOLOR_YELLOW "\033[1;33m"
#define XCOLOR_BLUE "\033[1;34m"
#define XCOLOR_MAGENTA "\033[1;35m"
#define XCOLOR_CYAN "\033[1;36m"


/**
 * Module Method definitions.
 */

// Main init & helpers.
string getWindowManagerName();
bool canDisplayReportWMName();
Window getRootWindowFromDisplay();
string getWMNameFromRootWindow(Window rootWindow);

string getDisplayManagerName();
bool mergeRootImageUnderSplashImage(int xPos, int yPos);
XImage* createBlackXImage();

// Display & helpers.
void displaySplashImage();
bool hasUserCancelledSplash();
bool isMouseClickedAndHeldInWindow(Window splashImage);
void sleepWithTimeoutOrKbdPressed(Milliseconds
    timeoutValue);

// Framework & debug.
int handleX11ErrorEvent(Display* display,
    XErrorEvent* event);

void debugXImage(string tag, XImage* image);
void debugXAnyEvent(const XAnyEvent* event);
void debugXExposeEvent(const XExposeEvent* event);
