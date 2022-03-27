#include <WebServer.h>
#include <Update.h>


// Web server
WebServer server(80);

// Function default parameters
String html_header(String title, String meta = "");


/** Setup settings webserver */
void setupWebserver() {
  Serial.println("Configuring Webserver...");
  server.on("/", handleRoot);
  server.on("/color", HTTP_POST, handleColorSave);
  server.on("/wifi", HTTP_POST, handleWifiSave);
  server.on("/mqtt", HTTP_POST, handleMqttSave);
  server.on("/ota", HTTP_POST, handleOtaResponse, handleOtaUpload);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/style.css", handleCSS);
  server.onNotFound(handleNotFound);
  server.begin(); // Web server start
}

void webserver_loop() {
  //HTTP
  server.handleClient();
}



/** Webserver: Handle root or redirect to captive portal */
void handleRoot() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
    return;
  }

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  String Page;
  Page = html_header("test");
  Page += html_form_color();
  Page += html_form_wifi();
  Page += html_form_mqtt();
  Page += html_form_ota();
  Page += html_footer();

  server.send(200, "text/html", Page);
}

/** Handle the WLAN save form and reboot */
void handleWifiSave() {
  Serial.println("wifi save");
  ssid = server.arg("ssid");
  password = server.arg("pass");

  saveString("ssid", ssid);
  saveString("password", password);

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  String Page;
  Page = html_header("WIFI", "<meta http-equiv='refresh' content='10; URL=/'>");
  Page += F("De wifi-instellingen werden opgeslaan. Toestel wordt herstart... ");
  Page += html_footer();

  server.send(200, "text/html", Page);

  ESP.restart();
}

/** Handle the MQTT save form  */
void handleMqttSave() {
  Serial.println("MQTT save");

  mqtt_enabled = (server.arg("mqtt").toInt() == 1);
  mqtt_server = server.arg("mqtt_server");
  mqtt_port = server.arg("mqtt_port").toInt();
  mqtt_user = server.arg("mqtt_user");
  mqtt_pass = server.arg("mqtt_pass"); 

  saveBool("mqtt_enabled", mqtt_enabled);
  saveString("mqtt_server", mqtt_server);
  saveInt("mqtt_port", mqtt_port);
  saveString("mqtt_user", mqtt_user);
  saveString("mqtt_pass", mqtt_pass);

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  String Page;
  Page = html_header("MQTT", "<meta http-equiv='refresh' content='10; URL=/'>");
  Page += F("De MQTT-instellingen werden opgeslaan.  ");
  Page += html_footer();

  server.send(200, "text/html", Page);

  setup_mqtt(); 
}

void handleColorSave() {
  Serial.println("color save");
  Serial.println(server.arg("color"));
  //  server.arg("color").toInt(color);
  color = server.arg("color").toInt();
  //server.arg("ssid").toCharArray(ssid, sizeof(ssid) - 1);
  //server.arg("pass").toCharArray(password, sizeof(password) - 1);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  String Page;
  Page = html_header("Color", "<meta http-equiv='refresh' content='1; URL=/'>");
  Page += F("Saved... ");
  Page += html_footer();

  server.send(200, "text/html", Page);
}

/** Handle the OTA save form and reboot */
void handleOtaResponse() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  String Page;
  Page = html_header("OTA", "<meta http-equiv='refresh' content='10; URL=/'>");
  if (Update.hasError()) {
    Page += F("OTA failed");
  } else {
    Page += F("OTA succeeded.... Toestel wordt herstart... ");
  }
  Page += html_footer();

  server.send(200, "text/html", Page);
  if (!Update.hasError()) {
    ESP.restart();
  }
}

void handleOtaUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    /* flashing firmware to ESP*/
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) { //true to set the size to the current progress
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

/** Webserver: handle 404 */
void handleNotFound() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
    return;
  }
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");

  for (uint8_t i = 0; i < server.args(); i++) {
    message += String(F(" ")) + server.argName(i) + F(": ") + server.arg(i) + F("\n");
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(404, "text/plain", message);
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  if (!isIp(server.hostHeader())) {
    Serial.println("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

/** Is this an IP? */
boolean isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}


/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}


String html_header(String title, String meta) {
  String Page;
  Page = F("<html><head><title>" );
  Page += webserver_title;
  Page += F( "</title>");
  Page += F( "<link rel='stylesheet' type='text/css' href='/style.css'>");
  Page += meta;
  Page += F("</head><body><h1>");
  Page += webserver_title;
  Page +=  F("</h1>");
  return Page;
}



String html_form_color() {
  String Page;
  Page = F("<form method='POST' action='/color'>"
           "<h4>Kleur:</h4>"
           "<div class='rainbow'>"
           "<input type='range' min='0' max='65536' value='");
  Page += String(color);
  Page += F("' name='color'>"
            "</div>"
            "<br /><input type='submit' value='Opslaan'/></form>"
            "</form>" );
  return Page;
}

String html_form_wifi() {
  String Page;
  Page = F("<form method='POST' action='/wifi'>"
           "<h4>Verbinden met netwerk:</h4>"
           "<label for='ssid'>Netwerk</label>"
           "<select name='ssid' id='ssid'>"
           "<option></option>");
  int networks = WiFi.scanNetworks();
  for (int i = 0; i < networks; i++) {
    if ((String)ssid == WiFi.SSID(i)) {
      Page += String(F("<option selected value='")) + WiFi.SSID(i) + (F("'>")) + WiFi.SSID(i) + F("</option>");
    } else {
      Page += String(F("<option value='")) + WiFi.SSID(i) + (F("'>")) + WiFi.SSID(i) + F("</option>");
    }
  }

  Page += F( "</select>"
             "<br />"
             "<label for='pass'>Paswoord</label>"
             "<input type='password' placeholder='password' name='pass' id='pass' />"
             "<br /><input type='submit' value='Opslaan'/></form>"
             "</form>" );
  return Page;
}


String html_form_mqtt() {
  String Page;
  Page = ("<form method='POST' action='/mqtt'>"
           "<h4>Verbinden met MQTT:</h4>"
           "<input type='checkbox' value='1' name='mqtt' id='mqtt' " + String(mqtt_enabled?"checked=checked":"") + " /> <label for='mqtt'>Enabled</label> <br />"
           "<label for='mqtt_server'>Server</label>"
           "<input type='text' value='" + String(mqtt_server) + "' placeholder='MQTT Server' name='mqtt_server' id='mqtt_server' /><br />"
           "<label for='mqtt_port'>Port</label>"
           "<input type='number' value='" + String(mqtt_port) + "' placeholder='MQTT Port' name='mqtt_port' id='mqtt_port' /><br />"
           "<label for='mqtt_user'>User</label>"
           "<input type='text' value='" + String(mqtt_user) + "' placeholder='MQTT User' name='mqtt_user' id='mqtt_user' /><br />"
           "<label for='mqttt_pass'>Password</label>"
           "<input type='password' value='" + String(mqtt_pass) + "' placeholder='MQTT Password' name='mqttt_pass' id='mqttt_pass' /><br />"
           "<br /><input type='submit' value='Opslaan'/></form>"
           "</form>" );
  return Page;
}

String html_form_ota() {
  String Page;
  Page = F("<form method='POST' action='/ota' enctype='multipart/form-data'>"
           "<h4>Update:</h4>"
           "<label for='update'>Update file</label>"
           "<input type='file' name='update' id='update'>"
           "<br /><input type='submit' value='Verzenden'/></form>"
           "</form>" );
  return Page;
}

String html_footer() {
  String Page;
  Page = F("</body></html>" );
  return Page;
}


/** Handle the CSS request */
void handleCSS() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  String Page;
  Page = F("body {  }"
           "div.rainbow {width: 80vw; height: 20px; border-radius: 5px; background: linear-gradient( 90deg, rgba(255, 0, 0, 1) 0%, rgba(255, 154, 0, 1) 10%, rgba(208, 222, 33, 1) 20%, rgba(79, 220, 74, 1) 30%, rgba(63, 218, 216, 1) 40%, rgba(47, 201, 226, 1) 50%, rgba(28, 127, 238, 1) 60%, rgba(95, 21, 242, 1) 70%, rgba(186, 12, 248, 1) 80%, rgba(251, 7, 217, 1) 90%, rgba(255, 0, 0, 1) 100% ); display: flex;}"
           "div.rainbow input {width: 100%; height: 0; }");

  server.send(200, "text/css", Page);
}
