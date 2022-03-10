#include <EEPROM.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Update.h>

#include "credentials.h"

// DNS server
DNSServer dnsServer;

// Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM
char ssid[32] = "";
char password[32] = "";

// Soft Access Point IP
IPAddress apIP(8, 8, 8, 8);
//IPAddress apIP(192,168,1,1);


// Web server
WebServer server(80);

// Function default parameters
String html_header(String title, String meta = "");

/** MAIN FUNCTION: connect to the internet and setup settings webserver */
bool online() {
  bool connected = false;
  loadCredentials();
  if (strlen(ssid) > 0) {
    connected = connect_wifi(ssid, password);
  }
  if (!connected) {
    setupSoftAP();
    setupDNSserver();
  }
  setupWebserver();

  return connected;
}


/** Connect to wifi network */
bool connect_wifi(char ssid[], char password[]) {
  int loops = 0;
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++loops > 10) {
      Serial.println("");
      Serial.print("WiFi connection failed");
      return false;
    }
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  return true;
}

/** Load WLAN credentials from EEPROM */
void loadCredentials() {
  EEPROM.begin(512);
  EEPROM.get(0, ssid);
  EEPROM.get(0 + sizeof(ssid), password);
  char ok[2 + 1];
  EEPROM.get(0 + sizeof(ssid) + sizeof(password), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
  }
  Serial.println("Recovered credentials:");
  Serial.println(ssid);
  Serial.println(strlen(password) > 0 ? "********" : "<no password>");
}


/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  EEPROM.begin(512);
  EEPROM.put(0, ssid);
  EEPROM.put(0 + sizeof(ssid), password);
  char ok[2 + 1] = "OK";
  EEPROM.put(0 + sizeof(ssid) + sizeof(password), ok);
  EEPROM.commit();
  EEPROM.end();
}


/** Setup network */
void setupSoftAP () {
  IPAddress netMsk(255, 255, 255, 0);

  Serial.println("Configuring access point...");

  WiFi.softAP(softAP_ssid, "");
  WiFi.softAPConfig(apIP, apIP, netMsk);
  delay(500); // Without delay I've seen the IP address blank
}

/** Setup the DNS server redirecting all the domains to the apIP */
void setupDNSserver() {
  const byte DNS_PORT = 53;

  Serial.println("Configuring DNS server...");
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
}

/** Setup settings webserver */
void setupWebserver() {
  Serial.println("Configuring Webserver...");
  server.on("/", handleRoot);
  server.on("/wifisave", HTTP_POST, handleWifiSave);
  server.on("/ota", HTTP_POST, handleOtaResponse, handleOtaUpload);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.onNotFound(handleNotFound);
  server.begin(); // Web server start
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
  Page += html_form_wifi();
  Page += html_form_ota();
  Page += html_footer();

  server.send(200, "text/html", Page);
}

/** Handle the WLAN save form and reboot */
void handleWifiSave() {
  Serial.println("wifi save");
  server.arg("ssid").toCharArray(ssid, sizeof(ssid) - 1);
  server.arg("pass").toCharArray(password, sizeof(password) - 1);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  String Page;
  Page = html_header("WIFI", "<meta http-equiv='refresh' content='10; URL=/'>");
  Page += F("De wifi-instellingen werden opgeslaan. Toestel wordt herstart... ");
  Page += html_footer();

  server.send(200, "text/html", Page);
  saveCredentials();
  ESP.restart();
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

/** LOOP function  */
void internet_loop() {
  //DNS
  dnsServer.processNextRequest();

  //HTTP
  server.handleClient();
}


String html_header(String title, String meta) {
  String Page;
  Page = F("<html><head><title>" );
  Page += webserver_title;
  Page += F( "</title>");
  Page += meta;
  Page += F("</head><body><h1>");
  Page += webserver_title;
  Page +=  F("</h1>");
  return Page;
}

String html_form_wifi() {
  String Page;
  Page = F("<form method='POST' action='/wifisave'>"
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
