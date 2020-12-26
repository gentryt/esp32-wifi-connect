#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <SimpleDHT.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

#define pinDHT11 4
#define SensorPin 34
#define CalibrationLED 2

#define network "2.4GHz_Modem"
#define pw "L30N@rd01"
#define timeout 20000
#define REPORT_STATUS "garden/1/status"
#define CalibrateMoistureSensor "garden/1/calibrateMoistureSensor"

EEPROMClass  AirValue("eeprom0", 0x500);
EEPROMClass  WaterValue("eeprom1", 0x500);

String deviceName = "AutoWater_1";
const char* mqttServer = "192.168.1.75";
WiFiClient espClient;
WiFiUDP ntpUDP;
PubSubClient client(espClient);
NTPClient timeClient(ntpUDP,21600);

SimpleDHT11 dht11(pinDHT11);
int airValue;
int waterValue;
int intervals;
bool CalibratingMoistureSensor = false;

void setupEeprom(){
  Serial.println("Testing EEPROMClass");
  if (!AirValue.begin(AirValue.length())) {
    Serial.println("Failed to initialise AirValue");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (!WaterValue.begin(WaterValue.length())) {
    Serial.println("Failed to initialise WaterValue");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }

  AirValue.get(0, airValue);
  WaterValue.get(0, waterValue);
  Serial.print("Stored Air Value: "); Serial.println(airValue);
  Serial.print("Stored Water Value: "); Serial.println(waterValue);
}

void calibrateMoistureSensorValue(char* value){
  Serial.print("Made it to the calibration method: ");
  Serial.println((char*)value);
  if (strcmp(value, "air") != 0 && strcmp(value, "water") != 0){
    Serial.print("Invalid sensor calibration payload ");
    Serial.println((char*)value);
    return;
  }

  digitalWrite(CalibrationLED, HIGH);
  CalibratingMoistureSensor = true;
  Serial.print("Calibrating ");
  Serial.print((char*)value);
  Serial.println(" value");

  int readings = 0;
  for (int i = 0; i < 10 ; i++)
  {
    int reading = analogRead(SensorPin);
    Serial.print("Reading ");
    Serial.print(i+1);
    Serial.print(" is -");
    Serial.print(reading);
    Serial.println("-");

    readings += reading;
    delay(2500);
  }

  int avg = readings/10;

  if (strcmp(value, "air") == 0){
    Serial.print("The calibrated air value, should be ");
    Serial.println(avg);
    airValue = avg;
    AirValue.put(0, airValue);
     AirValue.commit();
  } else if (strcmp(value, "water") == 0){
    Serial.print("The calibrated water value, should be ");
    Serial.println(avg);
    waterValue = avg;
    WaterValue.put(0, waterValue);
     WaterValue.commit();
  } 
 
  CalibratingMoistureSensor = false;
  digitalWrite(CalibrationLED, LOW);
}

void receivedCallBack(char* topic, byte* payload, unsigned int length){
  Serial.print("Message received: ");
  Serial.println(topic);
  
  if (strcmp(topic, CalibrateMoistureSensor) == 0)
  {
    payload[length] = '\0';
    calibrateMoistureSensorValue((char*)payload);
  }
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
    delay(1000);
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
  delay(500);
}

void connectMqtt(){
  while (!client.connected())  {
    Serial.print("MQTT Connecting ...");
    String clientId ="ESP32Client";
    if (client.connect(clientId.c_str())){
      Serial.println("Connected!");
      client.subscribe(CalibrateMoistureSensor);
    }else{
      Serial.print("failed, status code =");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      
      delay(5000);
    }
  }

  Serial.println();
}

void reportStatus(){
  Serial.println("==========================================");
  Serial.println("Sample DHT11 and Moisture readings...");

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
  soilMoisturePercent = map(soilMoisture, airValue, waterValue, 0,100);
  int p = 0;
  if (soilMoisturePercent > 100){
    p = 100;
  }else if(soilMoisturePercent <= 0){
    p = 0;
  }else{
    p = soilMoisturePercent;
  }

  DynamicJsonDocument doc(256);
  doc["time"] = String(timeClient.getFormattedTime());
  doc["temp"] = String(temperature);
  doc["hum"] = String(humidity);
  doc["moisture"] = String(soilMoisture);
  doc["moisturePercent"] = String(p);
  doc["waterValue"] = String(waterValue);
  doc["airValue"] = String(airValue);

  char buffer[256];  
  serializeJson(doc, buffer);
  Serial.println(buffer);
  client.publish(REPORT_STATUS, buffer);

  delay(2500);
}

void setup() {
  pinMode(CalibrationLED, OUTPUT);
  pinMode(pinDHT11, INPUT);
  pinMode(SensorPin, INPUT);
  Serial.begin(115200);
  connectWifi();
  client.setServer(mqttServer, 1883);
  client.setCallback(receivedCallBack);
  timeClient.begin();
  setupEeprom();
}

void loop() {
  timeClient.update();
  if (!client.connected()){
    connectMqtt();
  }
  client.loop();

  if (!CalibratingMoistureSensor) {
    reportStatus();
  }

}