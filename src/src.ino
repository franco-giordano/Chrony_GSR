#include "chrony/Chrony.h"

// Do not edit anything below this, leave all of your code above.
ChronyGSR watchy;

void setup(){
  Serial.begin(115200);
  watchy.init();
}

void loop(){}
