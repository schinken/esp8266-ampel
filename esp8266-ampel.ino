#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

#include "settings.h"
#include "page_header.h"
#include "page_footer.h"
#include "logo.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

IPAddress accessPointIP(192, 168, 1, 1);
IPAddress subnetMask(255, 255, 255, 0);

DNSServer dnsServer;
ESP8266WebServer webServer(80);

String currentScene = "manual";

uint8_t currentManualLed = -1;

unsigned long sceneSeconds;
unsigned long lastMillis;

uint8_t ampelMode = 0;

const uint8_t LED[] = {D7, D6, D5};

void lightsOff() {
  for (uint8_t i = 0; i < ARRAY_SIZE(LED); i++) {
    pinMode(LED[i], OUTPUT);
    digitalWrite(LED[i], LOW);
  }
}

void setLed(uint8_t led) {
  digitalWrite(LED[led], HIGH);
}

void sceneManual(unsigned long seconds) {
  lightsOff();
  setLed(currentManualLed);
}

void sceneAmpel(unsigned long seconds) {
  switch (seconds) {
    case 0: ampelMode = 0; break;
    case 10: ampelMode = 1; break;
    case 20: ampelMode = 2; break;
    case 40: ampelMode = 0; break;
  }

  lightsOff();
  
  switch (ampelMode) {
    case 0:
      setLed(0);
      break;

    case 1:
      setLed(0);
      setLed(1);
      break;

    case 2:
      setLed(2);
      break;
  }
}

void sceneDisco(unsigned long seconds) {
  lightsOff();

  for (uint8_t i = 0; i < ARRAY_SIZE(LED); i++) {
    if (random(0, 100) > 50) {
      setLed(i);
    }
  }
}

void sceneOff(unsigned long seconds){
  lightsOff();
}

void tickScene(unsigned long seconds) {
  if (currentScene == "manual") {
    sceneManual(seconds);
  } else if (currentScene == "ampel") {
    sceneAmpel(seconds);  
  } else if (currentScene == "disco") {
    sceneDisco(seconds);
  } else {
    sceneOff(seconds);
  }
}

void setScene(String scene) {
  currentScene = scene;
  sceneSeconds = 0;
  
  tickScene(0);
}

void handleIndex() {
  if (webServer.arg("led") != "") {
    currentManualLed = webServer.arg("led").toInt();
  }
  
  if (webServer.arg("scene") != "") {
    setScene(webServer.arg("scene"));
  }


  // If we use multiple sendContent, we have to send the header our self. 
  webServer.sendContent("HTTP/1.0 200 OK\r\n");
  webServer.sendContent("Connection: close\r\n");
  webServer.sendContent("Content-Type: text/html\r\n");
  webServer.sendContent("Cache-Control: no-store\r\n");
  webServer.sendContent("\r\n");
  
  webServer.sendContent_P( page_header_html, page_header_html_len);
  
  for (uint8_t i = 0; i < ARRAY_SIZE(LED); i++) {
    if (currentManualLed == i && currentScene == "manual") {
      webServer.sendContent("<a href=?scene=manual&led=" + String(i) + "><div id=led" + String(i) + " class=active></div></a>");
    } else {
      webServer.sendContent("<a href=?scene=manual&led=" + String(i) + "><div id=led" + String(i) + "></div></a>");
    }  
  }

  webServer.sendContent("</div><div id=scenes>");
  
  sendScene("off", "Aus");
  sendScene("ampel", "Ampel");
  sendScene("disco", "Disco");
  
  webServer.sendContent_P(page_footer_html, page_footer_html_len);
}

void sendScene(String key, String text) {
  webServer.sendContent("<a href=?scene=" + key + " " + ((currentScene == key)? "class=active" : "") + ">" + text + "</a>");
}



void setup() {

  for (uint8_t i = 0; i < ARRAY_SIZE(LED); i++) {
    pinMode(LED[i], OUTPUT);
    digitalWrite(LED[i], LOW);
  }
  
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(accessPointIP, accessPointIP, subnetMask);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);

  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(53, "*", accessPointIP);


  webServer.on("/logo.png", []() {
    webServer.sendHeader("Cache-Control", "public, max-age=31536000");
    webServer.sendHeader("Expires", "Sat, 13 May 2040 07:00:00 GMT");
    webServer.send_P(200, "image/png", logo_png, logo_png_len);
  });
  
  webServer.on("/", handleIndex);
  webServer.onNotFound(handleIndex);
  
  webServer.begin();

  randomSeed(analogRead(0));
  
  sceneSeconds = 0;
  lastMillis = millis();
}

void loop() {
  if (millis() - lastMillis > 1000) {
    lastMillis = millis();
    tickScene(sceneSeconds++);
  }
  
  dnsServer.processNextRequest();
  webServer.handleClient();  
}
