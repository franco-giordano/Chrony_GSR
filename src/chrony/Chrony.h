#include "Watchy_GSR.h"

#include "assets/smText8pt7b.h"
#include "assets/timeLG20pt7b.h"
#include "assets/timeLG42pt7b.h"
#include "assets/icons.h"

#include "Defines_Secrets.h"

// Place all of your data and variables here.

RTC_DATA_ATTR uint8_t ChronyStyle; // Remember RTC_DATA_ATTR for your variables so they don't get wiped on deep sleep.
extern int GuiMode;
extern struct Optional final
{
    bool TwentyFour;         // If the face shows 24 hour or Am/Pm.
    bool LightMode;          // Light/Dark mode.
    bool Feedback;           // Haptic Feedback on buttons.
    bool Border;             // True to set the border to black/white.
    bool Lefty;              // Swaps the buttons to the other side.
    bool Swapped;            // Menu and Back buttons swap ends (vertically).
    bool Orientated;         // Set to false to not bother which way the buttons are.
    uint8_t Turbo;           // 0-10 seconds.
    uint8_t MasterRepeats;   // Done for ease, will be in the Alarms menu.
    int Drift;               // Seconds drift in RTC.
    bool UsingDrift;         // Use the above number to add to the RTC by dividing it by 1000.
    uint8_t SleepStyle;      // 0==Disabled, 1==Always, 2==Sleeping
    uint8_t SleepMode;       // Turns screen off (black, won't show any screen unless a button is pressed)
    uint8_t SleepStart;      // Hour when you go to bed.
    uint8_t SleepEnd;        // Hour when you wake up.
    uint8_t Performance;     // Performance style, "Turbo", "Normal", "Battery Saving"
    bool NeedsSaving;        // NVS code to tell it things have been updated, so save to NVS.
    bool BedTimeOrientation; // Make Buttons only work while Watch is in normal orientation.
    uint8_t WatchFaceStyle;  // Using the Style values from Defines_GSR.
    uint8_t LanguageID;      // The LanguageID.
} Options;
extern struct MenuUse final {
    int8_t Style;           // GSR_MENU_INNORMAL or GSR_MENU_INOPTIONS
    int8_t Item;            // What Menu Item is being viewed.
    int8_t SubItem;         // Used for menus that have sub items, like alarms and Sync Time.
    int8_t SubSubItem;      // Used mostly in the alarm to offset choice.
} Menu;
extern struct CountUp final {
  bool Active;
  time_t SetAt;
  time_t StopAt;
} TimerUp;

RTC_DATA_ATTR char ALERT_BUFFER[50];
RTC_DATA_ATTR int WIFI_USAGE_COUNTER = 0;
RTC_DATA_ATTR HTTPClient autoremoteCli;
RTC_DATA_ATTR int httpResponseCode;

void setAlert(const char *msg)
{
    Serial.print("SETTING ALERT: ");
    Serial.println(msg);
    if (!msg)
    {
        memset(ALERT_BUFFER, 0, sizeof(ALERT_BUFFER));
    }
    else
    {
        strcpy(ALERT_BUFFER, msg);
    }
}

class ChronyGSR : public WatchyGSR
{
public:
    ChronyGSR() : WatchyGSR() {}

    void InsertPost()
    {
        // for some reason, GSR marks my RTC as dead on boot
        // because of this, my top right button won't work
        // and performance will be set to 'battery saving'
        // force-set it to false to avoid this
        WatchTime.DeadRTC = false;
        Options.Feedback = true;
        // Options.SleepStyle = 0;
    };

    void InsertDefaults()
    {
        AllowDefaultWatchStyles(false);
    }

    // The next 3 functions allow you to add your own WatchFaces, there are examples that do work below.

    void InsertAddWatchStyles()
    {
        ChronyStyle = AddWatchStyle("Chrony");
    };

    void InsertInitWatchStyle(uint8_t StyleID)
    {
        if (StyleID == ChronyStyle)
        {
            Design.Menu.Top = 72;
            Design.Menu.Header = 25;
            Design.Menu.Data = 66;
            Design.Menu.Gutter = 3;
            Design.Menu.Font = &aAntiCorona12pt7b;
            Design.Menu.FontSmall = &aAntiCorona11pt7b;
            Design.Menu.FontSmaller = &aAntiCorona10pt7b;
            Design.Face.Bitmap = nullptr;
            Design.Face.SleepBitmap = rtcsleep;
            Design.Face.Gutter = 4;
            Design.Face.Time = 56;
            Design.Face.TimeHeight = 45;
            Design.Face.TimeColor = GxEPD_BLACK;
            Design.Face.TimeFont = &timeLGMono42pt7b;
            Design.Face.TimeLeft = 0;
            Design.Face.TimeStyle = WatchyGSR::dCENTER;
            Design.Face.Day = 101;
            Design.Face.DayGutter = 4;
            Design.Face.DayColor = GxEPD_BLACK;
            Design.Face.DayFont = &timeLGMono20pt7b;
            Design.Face.DayFontSmall = &timeLGMono20pt7b;
            Design.Face.DayFontSmaller = &timeLGMono20pt7b;
            Design.Face.DayLeft = 0;
            Design.Face.DayStyle = WatchyGSR::dCENTER;
            Design.Face.Date = 143;
            Design.Face.DateGutter = 4;
            Design.Face.DateColor = GxEPD_BLACK;
            Design.Face.DateFont = &smTextMono8pt7b;
            Design.Face.DateFontSmall = &smTextMono8pt7b;
            Design.Face.DateFontSmaller = &smTextMono8pt7b;
            Design.Face.DateLeft = 0;
            Design.Face.DateStyle = WatchyGSR::dCENTER;
            Design.Face.Year = 186;
            Design.Face.YearLeft = 99;
            Design.Face.YearColor = GxEPD_BLACK;
            Design.Face.YearFont = &smTextMono8pt7b;
            Design.Face.YearLeft = 0;
            Design.Face.YearStyle = WatchyGSR::dCENTER;
            Design.Status.WIFIx = 5;
            Design.Status.WIFIy = 193;
            Design.Status.BATTx = 155;
            Design.Status.BATTy = 178;
        }
    };

    void InsertDrawWatchStyle(uint8_t StyleID)
    {
        if (StyleID == ChronyStyle)
        {
            Serial.print("STATE IS ");
            Serial.println(String(GuiMode));
            Serial.print("WIFI IS: ");
            Serial.println(currentWiFi());
            if (SafeToDraw())
            {
                drawTime();
                drawYear();
                drawDay();
            }
            if (NoMenu())
                drawDate();
            if (ALERT_BUFFER[0]) // there is an alert
            {
                int16_t x1, y1, z1;
                uint16_t w, h;
                display.drawBitmap(0, Design.Menu.Top, MenuBackground, GSR_MenuWidth, GSR_MenuHeight, ForeColor(), BackColor());
                setFontFor("Alert", Design.Menu.Font, Design.Menu.FontSmall, Design.Menu.FontSmaller, Design.Menu.Gutter);
                display.getTextBounds("Alert", 0, Design.Menu.Header + Design.Menu.Top, &x1, &y1, &w, &h);
                w = (196 - w) / 2;
                display.setCursor(w + 2, Design.Menu.Header + Design.Menu.Top);
                display.print("Alert");

                setFontFor(ALERT_BUFFER, Design.Menu.Font, Design.Menu.FontSmall, Design.Menu.FontSmaller, Design.Menu.Gutter);
                display.getTextBounds(ALERT_BUFFER, 0, Design.Menu.Data + Design.Menu.Top, &x1, &y1, &w, &h);
                w = (196 - w) / 2;
                display.setCursor(w + 2, Design.Menu.Data + Design.Menu.Top);
                display.print(ALERT_BUFFER);
            }
        }
    };

    bool InsertHandlePressed(uint8_t SwitchNumber, bool &Haptic, bool &Refresh)
    {
        switch (SwitchNumber)
        {
        case 2:             // Back
            Serial.println("OVERRIDE BACK!");
            // Simulate we are on TIMEUP menu
            GuiMode = GSR_MENUON;
            Menu.Style = GSR_MENU_INTIMERS;
            Menu.Item = GSR_MENU_TIMEUP;
            Menu.SubItem = 1;

            handleButtonPress(1); // turn on

            Haptic = true;  // Cause Hptic feedback if set to true.
            Refresh = true; // Cause the screen to be refreshed (redrwawn).
            return true;    // Respond with "I used a button", so the WatchyGSR knows you actually did something with a button.
            break;
        case 3: // Up
            return true;
            break;
        case 4: // Down
            Serial.println("OVERRIDE ABAJO!");
            if (GuiMode == GSR_WATCHON)
            {
                if (launchTuyaControl())
                {
                    Refresh = true;
                    Haptic = true;
                    return true;
                }
            }
        }
        return false;
    };

    bool OverrideSleepBitmap()
    {
        Serial.println("OVERRIDING BITMAP");
        display.setFont(&smTextMono8pt7b);
        display.setTextColor(GxEPD_WHITE);
        drawData("Watchy is asleep", Design.Face.TimeLeft, Design.Face.Time, WatchyGSR::dCENTER, 0);
        display.drawBitmap(100 - 45/2, 100 - 40/2 , Design.Face.SleepBitmap, 45, 40, GxEPD_WHITE, GxEPD_BLACK);
        drawData("Tap to wake", Design.Face.DateLeft, Design.Face.Date + 5, WatchyGSR::dCENTER, 0);
        return true;
    };

    // bool OverrideBitmap()
    // {
    //     return OverrideSleepBitmap();
    // };

    // http control
    bool launchTuyaControl()
    {
        if (WIFI_USAGE_COUNTER > 0)
        {
            // already in progress! dismiss
            return false;
        }
        setAlert("Asking for WiFi...");
        WIFI_USAGE_COUNTER = 1;
        AskForWiFi();
        return true;
    }

    void InsertWiFi()
    {
        Serial.println("CALLED INSERTWIFI");
        if (WIFI_USAGE_COUNTER == 0)
        {
            // I didn't ask for this, quit!
            return;
        }

        if (WIFI_USAGE_COUNTER == 1) // do step 1: begin
        {
            setAlert("Initializing...");
            showWatchFace();
            autoremoteCli.setConnectTimeout(3000); // 3 second max timeout
            autoremoteCli.begin(AUTOREMOTE_GET_URL);
            WIFI_USAGE_COUNTER++;
        }
        else if (WIFI_USAGE_COUNTER == 2) // step 2: query
        {
            setAlert("Sending...");
            showWatchFace();
            httpResponseCode = autoremoteCli.GET();
            WIFI_USAGE_COUNTER++;
        }
        else if (WIFI_USAGE_COUNTER == 3) // step 3:
        {
            if (httpResponseCode == 200 && autoremoteCli.getString() == "OK")
            {
                setAlert("Sent!");
                showWatchFace();
            }
            else
            {
                // http error
                Serial.println("HTTP or Tasker error :(");
                setAlert("Network or Tasker error :(");
                showWatchFace();
            }
            autoremoteCli.end();
            WIFI_USAGE_COUNTER++;
            // finished!
            endWiFi();
            setAlert(null);
        }
    }

    void InsertWiFiEnding()
    {
        WIFI_USAGE_COUNTER = 0;
    }
};
