#include <WiFi.h>
#include <DNSServer.h>

// DNS server
DNSServer dnsServer;

// Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM
String ssid;
String password;

// Soft Access Point IP
IPAddress apIP(8, 8, 8, 8);
//IPAddress apIP(192,168,1,1);


/** MAIN FUNCTION: connect to the internet and setup settings webserver */
bool online() {
  bool connected = false;
  ssid = loadString("ssid"); 
  password = loadString("password"); 
  if (strlen(ssid.c_str()) > 0) {
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
bool connect_wifi(String ssid, String password) {
  int loops = 0;
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.println(password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++loops > 10) {
      Serial.println("");
      Serial.println("WiFi connection failed");
      return false;
    }
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  return true;
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


/** LOOP function  */
void loop_internet() {
  //DNS
  dnsServer.processNextRequest();

  webserver_loop(); 
}
