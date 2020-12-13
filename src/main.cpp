#include <Arduino.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>

String deviceName = "AutoWater_1";
Preferences settings;
const char* mqttServer = "192.168.1.75";
WiFiClient espClient;
PubSubClient client(espClient);

#define network "2.4GHz_Modem"
#define pw "L30N@rd01"
#define timeout 20000

#define REPORT_TOPIC "garden/AutoWater_1/report"

void receivedCallBack(char* topic, byte* payload, unsigned int length){
  Serial.print("Message received: ");
  Serial.println(topic);
  Serial.print("Payload: ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  
  Serial.println();

  // Do work here
}

void  connectWifi(){
  Serial.println();
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
    Serial.println("Connected to WiFi!");
    Serial.print("IP Address - ");
    Serial.println(WiFi.localIP());
    Serial.println("Device Name: " + deviceName);
  }  
  Serial.println();
}

void connectMqtt(){
  while (!client.connected())  {
    Serial.print("MQTT Connecting ...");
    String clientId ="ESP32Client";
    if (client.connect(clientId.c_str())){
      Serial.println("Connected!");
      client.subscribe(REPORT_TOPIC);
    }else{
      Serial.print("failed, status code =");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      
      delay(5000);
    }
  }

  Serial.println();
}

void setupPreferences(){
  settings.begin("set app name", false); // false makes read/write
  settings.getString("keyname", "default if key doesn't exist"); // Get setting value
  settings.putString("keyname","newValue");
  settings.end();
}

void setup() {
  Serial.begin(115200);
  connectWifi();
  client.setServer(mqttServer, 1883);
  client.setCallback(receivedCallBack);
}

void loop() {
  if (!client.connected()){
    connectMqtt();
  }
  client.loop();



  
}