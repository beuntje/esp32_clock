#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

bool mqtt_enabled = false;
String mqtt_server = "";
int mqtt_port = 1883;
String mqtt_user = "";
String mqtt_pass = "";

void setup_mqtt() {
  mqtt_enabled = loadBool("mqtt_enabled");
  mqtt_server = loadString("mqtt_server");
  mqtt_port = loadInt("mqtt_port");
  mqtt_user = loadString("mqtt_user");
  mqtt_pass = loadString("mqtt_pass");

  mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
  mqttClient.setCallback(mqtt_callback);
  if (mqtt_enabled)   mqtt_reconnect();
}

bool mqtt() {
  return (mqtt_enabled && mqttClient.connected());
}

void loop_mqtt() {
  if (mqtt_enabled) {
  mqttClient.loop();
  if (!mqttClient.connected()) { 
    mqtt_reconnect();
  }
  }
}

void mqtt_reconnect() {
  // Try to reconnected
  if (!mqttClient.connected() && WiFi.status() == WL_CONNECTED) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {
      Serial.println("connected");
      mqttClient.subscribe("klokTijn/#");
      mqttClient.publish("klokTijn/msg", "Hello");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      mqtt_enabled = false; 
    }
  }
}


void mqtt_callback(char* topic, byte *payload, unsigned int length) {
  Serial.print("MQTT ");
  Serial.print(topic);
  Serial.print(": '");
  Serial.write(payload, length);
  Serial.println("'");

  if (strcmp(topic, "klokTijn/color") == 0) {
    int values[3] = {0, 0, 0};
    int c = 0;
    for (int i = 0; i < length; i++) {
      int v = payload[i] - 48;
      if (v >= 0 && v <= 9) {
        values[c] = values[c] * 10 + v;
      } else {
        c++;
      }
    }

    color = getHue(values[0], values[1], values[2]);

  }


}

int getHue(int red, int green, int blue) {
  float R = (float)red / 255;
  float G = (float)green / 255;
  float B = (float)blue / 255;

  float maxVal = R;
  float minVal = R;
  if (G > maxVal) maxVal = G;
  if (G < minVal) minVal = G;
  if (B > maxVal) maxVal = B;
  if (B < minVal) minVal = B;

  float hue;
  float pi = 3.1415;

  if (R == maxVal) {
    hue = (G - B) / (maxVal - minVal);
  } else   if (G == maxVal) {
    hue = (pi / 3 * 2) + (B - R) / (maxVal - minVal);
  } else {
    hue = (pi / 3 * 4) + (R - G) / (maxVal - minVal) ;
  }

  return hue * 65536 / pi / 2;
}
