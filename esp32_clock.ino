
#include "credentials.h"

void setup() {
  Serial.begin(115200);
  setupSettings();

  setup_leds();
  if (online()) {
    setup_time();
    setup_mqtt();
  } else {

  }
}

void loop() {
  int nummerke = 0000;
  int sec = 0;
  float tijd = 0;
  int loopSec = 500;
  while (true) {
    tijd = timeval();
    sec = int(tijd * 100) % 100; 
    led_show(tijd, color, 65536 * (float(sec) / 60));
    loop_internet();
    loop_mqtt(); 
  }
}
