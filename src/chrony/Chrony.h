#include "Watchy_GSR.h"
#include "Defines_Secrets.h"

// Place all of your data and variables here.

RTC_DATA_ATTR uint8_t ChronyStyle; // Remember RTC_DATA_ATTR for your variables so they don't get wiped on deep sleep.
extern int GuiMode;

#define CHR_ALERTON 2
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
    };

    void InsertDefaults()
    {
        AllowDefaultWatchStyles(false);
        // Options.Feedback = true;
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
            Design.Face.SleepBitmap = nullptr;
            Design.Face.Gutter = 4;
            Design.Face.Time = 56;
            Design.Face.TimeHeight = 45;
            Design.Face.TimeColor = GxEPD_BLACK;
            Design.Face.TimeFont = &aAntiCorona36pt7b;
            Design.Face.TimeLeft = 0;
            Design.Face.TimeStyle = WatchyGSR::dCENTER;
            Design.Face.Day = 101;
            Design.Face.DayGutter = 4;
            Design.Face.DayColor = GxEPD_BLACK;
            Design.Face.DayFont = &aAntiCorona16pt7b;
            Design.Face.DayFontSmall = &aAntiCorona15pt7b;
            Design.Face.DayFontSmaller = &aAntiCorona14pt7b;
            Design.Face.DayLeft = 0;
            Design.Face.DayStyle = WatchyGSR::dCENTER;
            Design.Face.Date = 143;
            Design.Face.DateGutter = 4;
            Design.Face.DateColor = GxEPD_BLACK;
            Design.Face.DateFont = &aAntiCorona15pt7b;
            Design.Face.DateFontSmall = &aAntiCorona14pt7b;
            Design.Face.DateFontSmaller = &aAntiCorona13pt7b;
            Design.Face.DateLeft = 0;
            Design.Face.DateStyle = WatchyGSR::dCENTER;
            Design.Face.Year = 186;
            Design.Face.YearLeft = 99;
            Design.Face.YearColor = GxEPD_BLACK;
            Design.Face.YearFont = &aAntiCorona16pt7b;
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
                if (GuiMode != CHR_ALERTON)
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
                    Serial.println("SETTING GUI ALERT");
                    Refresh = true;
                    Haptic = true;
                    // GuiMode = CHR_ALERTON;
                    return true;
                }
            }
        }
        return false;
    };

    /*
        bool OverrideSleepBitmap(){
          return false;
        };
    */

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
