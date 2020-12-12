#include <Arduino.h>
#include "WiFi.h"

#define network "2.4GHz_Modem"
#define pw "L30N@rd01"
#define timeout 20000

void  connectWifi(){
  Serial.println();
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(network, pw);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout){
    Serial.print(".");
    delay(100);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("Connection Failed!");
  }else{
    Serial.println();
    Serial.print("Connected! - ");
    Serial.println(WiFi.localIP());
  }  
}

void setup() {
  Serial.begin(115200);
  connectWifi();
}

void loop() {
  // put your main code here, to run repeatedly:
}