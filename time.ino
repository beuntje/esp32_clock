#include "time.h"

void setup_time() {
  configTime(0, 0, ntpServer);
  configTzTime(timezone, ntpServer);
}

int timeval() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return 0;
  }
  return timeinfo.tm_hour * 100 + timeinfo.tm_min;
}
