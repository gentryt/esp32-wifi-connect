#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <SimpleDHT.h>
#include <ArduinoJson.h>

#define pinDHT11 2
#define SensorPin 34

#define network "2.4GHz_Modem"
#define pw "L30N@rd01"
#define timeout 20000
#define REPORT_STATUS "garden/1/status"
#define CalibrateMoistureSensor "garden/1/calibrateMoistureSensor"

String deviceName = "AutoWater_1";
const char* mqttServer = "192.168.1.75";
WiFiClient espClient;
WiFiUDP ntpUDP;
PubSubClient client(espClient);
NTPClient timeClient(ntpUDP,21600);

SimpleDHT11 dht11(pinDHT11);
int AirValue = 2640;
int WaterValue = 1650;
bool CalibratingMoistureSensor = false;

void calibrateMoistureSensorValue1(char* value){
  Serial.print("Made it to the calibration method: ");
  Serial.println((char*)value);
  if (strcmp(value, "air") != 0 && strcmp(value, "water") != 0){
    Serial.print("Invalid sensor calibration payload ");
    Serial.println((char*)value);
    return;
  }

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
    AirValue = avg;
  } else if (strcmp(value, "water") == 0){
    Serial.print("The calibrated water value, should be ");
    Serial.println(avg);
    WaterValue = avg;
  }

    CalibratingMoistureSensor = false;
}

void calibrateMoistureSensorValue(char* value)
{
  std::string valueString = getenv(value);
  std::string air = getenv("air");
  std::string water = getenv("water");

  if (valueString != air && valueString != water)
  {
    Serial.println("Invalid sensor calibration payload " + String(value));
    Serial.println();
    return;
  }
  
  CalibratingMoistureSensor = true;
  Serial.println("Calibrating " + String(value) + " value");
  int readings = 0;
  for (int i = 0; i < 10 ; i++)
  {
    int reading = analogRead(SensorPin);
    readings += reading;
    Serial.print("Reading #" + i);
    Serial.println(reading);

    delay(2500);
  }
  
  int avg = readings/10;
  Serial.print("Calibrated ");
  Serial.print(String(value));
  Serial.print(" value = ");
  Serial.println(avg);
  
  if (valueString == air)
  {
      AirValue = avg;
  } else if(valueString == water){
      WaterValue = avg;
  } 

  CalibratingMoistureSensor = false; 
}

void receivedCallBack(char* topic, byte* payload, unsigned int length){
  Serial.print("Message received: ");
  Serial.println(topic);
  
  if (strcmp(topic, CalibrateMoistureSensor) == 0)
  {
    payload[length] = '\0';
    calibrateMoistureSensorValue1((char*)payload);
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

  DynamicJsonDocument doc(256);
  doc["time"] = String(timeClient.getFormattedTime());
  doc["temp"] = String(temperature);
  doc["hum"] = String(humidity);
  doc["moisture"] = String(soilMoisture);
  doc["moisturePercent"] = String(p);
  doc["waterValue"] = String(WaterValue);
  doc["airValue"] = String(AirValue);

  char buffer[256];  
  serializeJson(doc, buffer);
  Serial.println(buffer);
  client.publish(REPORT_STATUS, buffer);

  delay(2500);
}

void loadAppSettings(){
 // AirValue = settings.getString("AirValue", "0").toInt();
  //WaterValue = settings.getString("WaterValue", "100").toInt();
}

void setup() {
  pinMode(pinDHT11, INPUT);
  pinMode(SensorPin, INPUT);
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

  if (!CalibratingMoistureSensor)
  {
    reportStatus();
  }

}