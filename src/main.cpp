#include <Arduino.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <SimpleDHT.h>
#include <ArduinoJson.h>

int pinDHT11 = 2;
SimpleDHT11 dht11(pinDHT11);
const int AirValue = 2640;
const int WaterValue = 1650;
const int SensorPin = 34;

String deviceName = "AutoWater_1";
Preferences settings;
const char* mqttServer = "192.168.1.75";
WiFiClient espClient;
WiFiUDP ntpUDP;
PubSubClient client(espClient);
NTPClient timeClient(ntpUDP,21600);

#define network "2.4GHz_Modem"
#define pw "L30N@rd01"
#define timeout 20000

#define REPORT_TOPIC "garden/AutoWater_1/report"
#define REPORT_STATUS "garden/1/status"

void receivedCallBack(char* topic, byte* payload, unsigned int length){
  Serial.print("Message received: ");
  Serial.println(topic);
  Serial.print("Payload: ");

  String payloadText = "";
  for (int i = 0; i < length; i++) {
    payloadText += (char)payload[i];
  }
  
  Serial.println( payloadText);
  Serial.println();
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

void reportStatus(){
  Serial.println("==========================================");
  Serial.println("Sample DHT11...");

  byte temperature = 0;
  byte humidity = 0;

  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); 
    Serial.println(err);
    delay(1000);
    return;
  }

  int soilMoisture = 0;
  int soilMoisturePercent = 0;
  soilMoisture = analogRead(SensorPin);
  soilMoisturePercent = map(soilMoisture, AirValue, WaterValue, 0,100);
  int p = 0;
  if (soilMoisturePercent > 100){
    p = 100;
  }else if(soilMoisturePercent <= 0){
    p = 0;
  }else{
    p = soilMoisturePercent;
  }

  DynamicJsonDocument doc(128);
  doc["time"] = String(timeClient.getFormattedTime());
  doc["temp"] = String(temperature);
  doc["hum"] = String(humidity);
  doc["moisture"] = String(soilMoisture);
  doc["moisturePercent"] = String(p);

  char buffer[256];  
  serializeJson(doc, buffer);
  Serial.println(buffer);
  client.publish(REPORT_STATUS, buffer);

  delay(2500);
}

void setup() {
  Serial.begin(115200);
  connectWifi();
  client.setServer(mqttServer, 1883);
  client.setCallback(receivedCallBack);
  timeClient.begin();
}

void loop() {
  timeClient.update();
  if (!client.connected()){
    connectMqtt();
  }
  client.loop();

  reportStatus();

}