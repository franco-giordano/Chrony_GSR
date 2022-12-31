#include "chrony/Chrony.h"

// Do not edit anything below this, leave all of your code above.
ChronyGSR watchy;

void setup(){
  Serial.begin(115200);
  while (!Serial);
  
  watchy.init();
}

void loop(){}
