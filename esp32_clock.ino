
void setup() {
  Serial.begin(115200);
  setup_leds();
  if (online()) {
    setup_time();
  } else {
    
  }
}

void loop() {
  // xx_loop();
  int nummerke = 0000;
  int hue = 0;
  while (true) {
    // hue = random(0, 5 * 65536);
    hue += 256; 
    hue %= 5 * 65536; 
    nummerke = timeval(); 
    for (int i = 0; i <= 300; i++) {
      internet_loop();
      led_show(nummerke, hue, hue);
    }
  }
}
