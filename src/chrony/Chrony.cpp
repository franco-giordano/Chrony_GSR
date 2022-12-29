#include "Chrony.h"

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
extern struct MenuUse final
{
    int8_t Style;      // GSR_MENU_INNORMAL or GSR_MENU_INOPTIONS
    int8_t Item;       // What Menu Item is being viewed.
    int8_t SubItem;    // Used for menus that have sub items, like alarms and Sync Time.
    int8_t SubSubItem; // Used mostly in the alarm to offset choice.
} Menu;

RTC_DATA_ATTR char ALERT_BUFFER[50];
RTC_DATA_ATTR int WIFI_USAGE_COUNTER = 0;
RTC_DATA_ATTR HTTPClient autoremoteCli;
RTC_DATA_ATTR int httpResponseCode;

// utils -- defined last
void setAlert(const char *msg);
String padWithZero(uint8_t value);

void ChronyGSR::InsertPost()
{
    // for some reason, GSR marks my RTC as dead on boot
    // because of this, my top right button won't work
    // and performance will be set to 'battery saving'
    // force-set it to false to avoid this
    WatchTime.DeadRTC = false;
    Options.Feedback = true;
    // Options.SleepStyle = 0;
};

void ChronyGSR::InsertDefaults()
{
    AllowDefaultWatchStyles(false);
    Options.LightMode = false;
    Options.Border = true;
}

// The next 3 functions allow you to add your own WatchFaces, there are examples that do work below.

void ChronyGSR::InsertAddWatchStyles()
{
    ChronyStyle = AddWatchStyle("Chrony");
};

void ChronyGSR::InsertInitWatchStyle(uint8_t StyleID)
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
        Design.Face.Gutter = 0;
        Design.Face.Time = 87;
        Design.Face.TimeHeight = 45;
        Design.Face.TimeColor = ForeColor();
        Design.Face.TimeFont = &timeLGMono42pt7b;
        Design.Face.TimeLeft = 16;
        Design.Face.TimeStyle = WatchyGSR::dSTATIC;
        Design.Face.DayLeft = 76; // for Day of Week String
        Design.Face.Day = 184;
        Design.Face.DayGutter = 0;
        Design.Face.DayColor = ForeColor();
        Design.Face.DayFont = &smTextMono8pt7b;
        Design.Face.DayFontSmall = &smTextMono8pt7b;
        Design.Face.DayFontSmaller = &smTextMono8pt7b;
        Design.Face.DayStyle = WatchyGSR::dSTATIC;
        Design.Face.DateLeft = 76; // for Month Str and Day number (e.g. 8 November)
        Design.Face.Date = 169;
        Design.Face.DateGutter = 0; // only use for Month Str
        Design.Face.DateColor = ForeColor();
        Design.Face.DateFont = &smTextMono8pt7b;
        Design.Face.DateFontSmall = &smTextMono8pt7b;
        Design.Face.DateFontSmaller = &smTextMono8pt7b;
        Design.Face.DateStyle = WatchyGSR::dSTATIC;
        Design.Face.YearLeft = 99; // for Year number (e.g. 2022)
        Design.Face.Year = 186;
        Design.Face.YearColor = ForeColor();
        Design.Face.YearFont = &smTextMono8pt7b;
        Design.Face.YearLeft = 0;
        Design.Face.YearStyle = WatchyGSR::dSTATIC;
        Design.Status.WIFIx = 144;
        Design.Status.WIFIy = 26;
        Design.Status.BATTx = 155;
        Design.Status.BATTy = 39;
    }
};

void ChronyGSR::InsertDrawWatchStyle(uint8_t StyleID)
{
    if (StyleID == ChronyStyle)
    {
        Serial.print("STATE IS ");
        Serial.println(String(GuiMode));
        Serial.print("WIFI IS: ");
        Serial.println(currentWiFi());

        drawChronyWatchStyle();

        drawAlert();
    }
};

bool ChronyGSR::InsertHandlePressed(uint8_t SwitchNumber, bool &Haptic, bool &Refresh)
{
    switch (SwitchNumber)
    {
    case 2: // Back
        Serial.println("OVERRIDE BACK!");
        if (GuiMode == GSR_WATCHON && StartTimerShortcut())
        {
            Refresh = true;
            Haptic = true;
            return true;
        }
        break;
    case 3: // Up
        return false;
        break;
    case 4: // Down
        Serial.println("OVERRIDE DOWN!");
        if (GuiMode == GSR_WATCHON && SendAutoRemote())
        {
            Refresh = true;
            Haptic = true;
            return true;
        }
    }
    return false;
};

bool ChronyGSR::OverrideSleepBitmap()
{
    Serial.println("OVERRIDING BITMAP");
    display.setFont(&smTextMono8pt7b);
    display.setTextColor(GxEPD_WHITE);
    drawData("Watchy is asleep", Design.Face.TimeLeft, 34, WatchyGSR::dCENTER, 0);
    display.drawBitmap(100 - 45 / 2, 100 - 40 / 2, Design.Face.SleepBitmap, 45, 40, GxEPD_WHITE, GxEPD_BLACK);
    drawData("Tap to wake", Design.Face.DateLeft, Design.Face.Date + 5, WatchyGSR::dCENTER, 0);
    return true;
};

void ChronyGSR::drawChronyWatchStyle()
{
    if (SafeToDraw())
    {
        drawDateTime();
        drawTemperature();

        // batt voltage
        display.setFont(Design.Face.DayFont); // reuse
        display.setTextColor(Design.Face.DayColor);
        drawData(String(getBatteryVoltage()), Design.Face.TimeLeft, 26, WatchyGSR::dSTATIC, 0);
    }
}

void ChronyGSR::drawDateTime()
{
    // drawTime();
    String hourStr = padWithZero(WatchTime.Local.Hour);
    String minStr = padWithZero(WatchTime.Local.Minute);

    display.setFont(Design.Face.TimeFont);
    display.setTextColor(Design.Face.TimeColor);
    drawData(hourStr, Design.Face.TimeLeft, Design.Face.Time, Design.Face.TimeStyle, 0);
    drawData(minStr, 16, 148, Design.Face.TimeStyle, 0);

    // drawDay();
    display.setFont(Design.Face.DayFont);
    display.setTextColor(Design.Face.DayColor);
    String dayName = LGSR.GetWeekday(Options.LanguageID, WatchTime.Local.Wday);

    String monthName = LGSR.GetMonth(Options.LanguageID, WatchTime.Local.Month);
    String dateStr = padWithZero(WatchTime.Local.Day);

    // day number (08)
    display.setFont(&timeLGMono20pt7b); // use different hardcoded values
    display.setCursor(16, 184);         // WatchyGSR assumes monthStyle == dateStyle
    display.print(dateStr);             // which is not the case here

    // month name (december)
    display.setFont(Design.Face.DateFont);
    display.setTextColor(Design.Face.DateColor);
    drawData(monthName, Design.Face.DateLeft, Design.Face.Date, Design.Face.DateStyle, 0);

    // week day (thursday)
    display.setFont(Design.Face.DayFont);
    display.setTextColor(Design.Face.DayColor);
    drawData(dayName, Design.Face.DayLeft, Design.Face.Day, Design.Face.DayStyle, 0);
}

void ChronyGSR::drawTemperature()
{
    String temperature = String(int(round(SBMA.readTemperature())));
    display.setFont(Design.Face.DayFont); // reuse
    display.setTextColor(Design.Face.DayColor);
    // Get width of text & center it under the weather icon. 165 is the centerpoint of the icon
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(temperature + ".", 45, 13, &x1, &y1, &w, &h);
    display.setCursor(166 - w / 2, 148);
    display.println(temperature + ".");

    const unsigned char *weatherIcon = WatchTime.BedTime ? clearskynight : clearsky;
    display.drawBitmap(143, 93, weatherIcon, 45, 40, ForeColor());
}

void ChronyGSR::drawAlert()
{
    if (ALERT_BUFFER[0]) // there is an alert
    {
        int16_t x1, y1, z1;
        uint16_t w, h;
        display.drawBitmap(0, Design.Menu.Top, MenuBackground, GSR_MenuWidth, GSR_MenuHeight, ForeColor(), BackColor());
        display.setTextColor(GxEPD_BLACK);
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

// http control
bool ChronyGSR::SendAutoRemote()
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

void ChronyGSR::InsertWiFi()
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

void ChronyGSR::InsertWiFiEnding()
{
    WIFI_USAGE_COUNTER = 0;
}

bool ChronyGSR::StartTimerShortcut()
{
    // Simulate we are on TIMEUP menu
    GuiMode = GSR_MENUON;
    Menu.Style = GSR_MENU_INTIMERS;
    Menu.Item = GSR_MENU_TIMEUP;
    Menu.SubItem = 1;

    handleButtonPress(1); // turn on

    return true;
}

/*
    === UTILS ===
*/

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

String padWithZero(uint8_t value)
{
    return value < 10 ? "0" + String(value) : String(value);
}
