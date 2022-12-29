#ifndef CHRONY_H
#define CHRONY_H

#include "Watchy_GSR.h"

#include "assets/smText8pt7b.h"
#include "assets/timeLG20pt7b.h"
#include "assets/timeLG42pt7b.h"
#include "assets/icons.h"

#include "Defines_Secrets.h"

class ChronyGSR : public WatchyGSR
{
public:
    ChronyGSR() : WatchyGSR() {}

private:
    // GSR Overrides
    void InsertPost();
    void InsertDefaults();
    void InsertAddWatchStyles();
    void InsertInitWatchStyle(uint8_t StyleID);
    void InsertDrawWatchStyle(uint8_t StyleID);
    bool OverrideSleepBitmap();
    bool InsertHandlePressed(uint8_t SwitchNumber, bool &Haptic, bool &Refresh);
    void InsertWiFi();
    void InsertWiFiEnding();

    // UI and Style
    void drawChronyWatchStyle();
    void drawDateTime();
    void drawTemperature();
    void drawAlert();

    // 'Apps'
    // http control
    bool SendAutoRemote();
};

#endif
