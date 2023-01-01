#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include "BLE.h"


class Notifications {
public:
    BLE BT;
    time_t lastGetTime = 0;
    void init() {
        BT.begin("Chrony BLE");
    }

    void deinit() {
        BT.deinitBLE();
    }

    bool getNotifications(String* dataPtr, time_t currentTime) {
        if (currentTime - lastGetTime > 5) { // fetch if 'old' data (> 5seconds)
            bool success =  BT.sendBLE("/notifications", dataPtr, true);
            if (success) lastGetTime = currentTime;
            return success;
        } else {
            return true;
        }
    }

};

#endif