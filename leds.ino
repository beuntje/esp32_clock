#include <Adafruit_NeoPixel.h>

#define LED_PIN    5
#define LED_COUNT 29

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

int eights[5][8] = {
  {5, 21, 16, 20, 4, 3, 15},
  {7, 23, 17, 22, 6, 2, 14},
  {9, 26, 18, 25, 8, 1, 13},
  {11, 28, 19, 27, 10, 0, 12},
  {24}
};

int chars[16][7] = {
  {1, 1, 1, 1, 1, 1, 0},  // 0
  {1, 1, 0, 0, 0, 0, 0}, // 1
  {1, 0, 1, 1, 0, 1, 1}, // 2
  {1, 1, 1, 0, 0, 1, 1}, // 3
  {1, 1, 0, 0, 1, 0, 1}, // 4
  {0, 1, 1, 0, 1, 1, 1}, // 5
  {0, 1, 1, 1, 1, 1, 1}, // 6
  {1, 1, 0, 0, 0, 1, 0}, // 7
  {1, 1, 1, 1, 1, 1, 1}, // 8
  {1, 1, 1, 0, 1, 1, 1}, // 9
  {0, 0, 0, 0, 0, 0, 0}, // ' ' // 10
  {0, 0, 0, 1, 1, 1, 1}, // 'F' // 11
  {0, 0, 1, 1, 1, 0, 1}, // 't' // 12
  {0, 0, 0, 1, 1, 0, 0}, // 'I' // 13
  {1, 1, 0, 0, 1, 1, 1}, // 'q' // 14
  {0, 0, 1, 0, 0, 0, 0} // '_' // 15
};

void setup_leds() {
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
}

void led_show(int nummer, int hue_numbers, int hue_dot) {
  uint32_t color = strip.gamma32(strip.ColorHSV(hue_numbers)); // hue -> RGB
  strip.clear();
  for (int eight = 0; eight < 4; eight++) {
    int value = int(nummer / pow(10, 3 - eight)) % 10;
    for (int led = 0; led < 7; led++) {
      if (chars[value][led])    {
        strip.setPixelColor(eights[eight][led], color);
      }
    }
  }  
   
  color = strip.gamma32(strip.ColorHSV(hue_dot)); 
  strip.setPixelColor(eights[4][0], color);
  strip.show();
}
