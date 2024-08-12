#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <esp_now.h>
#include <WiFi.h>
#include <AntaresESPHTTP.h>
#include <ArduinoJson.h> // Include ArduinoJson library

#define ACCESSKEY "52611abd7352ee93:5b27d91bb8c356e7" // Antares account access key
#define WIFISSID "E5576_AFF2" // Wi-Fi SSID
#define PASSWORD "25LndEd2G97" // Wi-Fi password
#define projectName "IoTBawangMerah_gen2" // Antares application name 
#define deviceName "data_sensor" // Antares device name 
#define deviceNameControl "data_control" // antares device 2 name 
const unsigned long interval = 15000; // Interval to send sensor data (10 Minute)
const unsigned long interval2 = 10000; // Interval to get data from Antares (10 Second)
unsigned long previousMillis = 0; // Store the last time sensor data was sent
unsigned long previousMillis2 = 0; // Store the last time data was received from Antares
AntaresESPHTTP antares(ACCESSKEY);

#define RE 2
#define DE 4
#define RX2 16
#define TX2 17

const byte nitro[] = {0x01,0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte phos[] = {0x01,0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte pota[] = {0x01,0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};

byte values[11];

void setup() {
  Serial.begin(115200);
  antares.setDebug(true);
  antares.wifiConnection(WIFISSID, PASSWORD);

  Serial2.begin(4800, SERIAL_8N1, RX2, TX2);
   
  // Initialize ADC pins
  pinMode(32, INPUT);
  pinMode(33, INPUT);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);

  checkData();
  
}
float readPH() {
  const int numReadings = 20;
  int sensorValues[numReadings];
  // Read the ADC value 20 times
  for (int i = 0; i < numReadings; i++) {
    sensorValues[i] = analogRead(32);
    delay(30); // Short delay between readings
  }
  // Sort the array
  for (int i = 0; i < numReadings - 1; i++) {
    for (int j = i + 1; j < numReadings; j++) {
      if (sensorValues[i] > sensorValues[j]) {
        int temp = sensorValues[i];
        sensorValues[i] = sensorValues[j];
        sensorValues[j] = temp;
      }
    }
  }
  // Compute the average of the middle 11 values
  float averageValue = 0;
  for (int i = 4; i <= 14; i++) {
    averageValue += sensorValues[i];
  }
  averageValue /= 11;
  // Convert the average ADC value to pH value
  averageValue = constrain(averageValue, 0.0, 14.0);
  float outputValuePH = (-0.0693 * averageValue) + 7.3855 + (float)random(1, 5)/100;

  // Calibrate and constrain the pH value
  outputValuePH = constrain(outputValuePH, 0.0, 14.0);
  return outputValuePH;
}
int readSoilMoisture() {
  int sensorValueMoisture = analogRead(33);
  int moisturePercent = (100 - ((sensorValueMoisture / 4095.00) * 100));
  moisturePercent = constrain(moisturePercent, 0, 100);
  return moisturePercent;
}


byte nitrogen(){
  delay(250);
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(Serial2.write(nitro,sizeof(nitro))==8){
    Serial2.flush();
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(Serial2.read(),HEX);
    values[i] = Serial2.read();
    Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}
 
byte phosphorous(){
  delay(250);
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(Serial2.write(phos,sizeof(phos))==8){
    Serial2.flush();
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(Serial2.read(),HEX);
    values[i] = Serial2.read();
    Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}
 
byte potassium(){
  delay(250);
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(Serial2.write(pota,sizeof(pota))==8){
    Serial2.flush();
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(Serial2.read(),HEX);
    values[i] = Serial2.read();
    Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}

void checkData() {
  Serial.println("checking Data");
  Serial.println(readSoilMoisture());
  Serial.println(readPH());
  Serial.println(nitrogen());
  Serial.println(phosphorous());
  Serial.println(potassium());
}


void sendToAntares() {
  antares.add("id_device", "plant001");
  antares.add("suhu", "-");
  antares.add("kelembapan_udara", "-");
  antares.add("kelembapan_tanah", readSoilMoisture());
  antares.add("ph_tanah", readPH());
  antares.add("nitrogen", nitrogen());
  antares.add("fosfor", phosphorous());
  antares.add("pottasium", potassium());

  // Send data to Antares
  antares.send(projectName, deviceName);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    sendToAntares();
  }
  //ESP Slave mengambil data secara konstan untuk memastikan bahwa esp selalu berkomunikasi dengan antares supaya powerbank selalu aktif.
  if(millis() - previousMillis2 > interval2) { // Check if it's time to get data from Antares
    previousMillis2 = millis(); // Update the last received time
    // antares.get(projectName, deviceName);
    delay(1000);
  }
}

