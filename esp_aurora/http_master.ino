#include <Wire.h>
// #include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <esp_now.h>
#include <WiFi.h>
#include <AntaresESPHTTP.h>
#include <ArduinoJson.h> // Include ArduinoJsonlibrary
#include <esp_now.h>

#define ACCESSKEY "52611abd7352ee93:5b27d91bb8c356e7" // Antares account access key
#define WIFISSID "E5576_AFF2" // Wi-Fi SSID
#define PASSWORD "25LndEd2G97" // Wi-Fi password
#define projectName "IoTBawangMerah_gen2" // Antares application name that was created
#define deviceName "data_sensor" // Antares device name that was created
#define deviceNameControl "data_control" // antares device 2 name that was created
#define RELAY_PIN1 13 // Define the pin number for the relay
#define RELAY_PIN2 16 // Define the pin number for the relay
Adafruit_BME280 bme; // Initialize BME280 sensor
AntaresESPHTTP antares(ACCESSKEY);
const unsigned long interval = 15000; // Interval to send sensor data (10 Minute)
const unsigned long interval2 = 1000; // Interval to get data from Antares (1 minute)
unsigned long previousMillis = 0; // Store the last time sensor data was sent
unsigned long previousMillis2 = 0; // Store the last time data was received from Antares

void setup() {
  Serial.begin(115200);
  antares.setDebug(true);
  antares.wifiConnection(WIFISSID, PASSWORD);

  // Initialize BME280
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  pinMode(RELAY_PIN1, OUTPUT); // Initialize the relay pin as an output
  pinMode(RELAY_PIN2, OUTPUT); // Initialize the relay pin as an output
  digitalWrite(RELAY_PIN1, HIGH); // Set the relay to HIGH by default
  digitalWrite(RELAY_PIN2, HIGH); // Set the relay to HIGH by default 
  delay(2000);
  sendToAntares();
  delay(2000);
}
void sendToAntares() {

  //tipe data supaya telekomunikasi gampang
  // Add sensor data to storage buffer
  antares.add("id_device", "master");
  antares.add("suhu", bme.readTemperature());
  antares.add("kelembapan_udara", bme.readHumidity());
  antares.add("kelembapan_tanah", "-");
  antares.add("ph_tanah", "-");
  antares.add("fosfor", "-");
  antares.add("nitrogen", "-");
  antares.add("pottasium", "-");

  // Send from buffer to Antares
  antares.send(projectName, deviceName);
}
void loop() {
  // delay(1000);
  unsigned long currentMillis = millis();
  // Send data to Antares
  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    sendToAntares();
  }
  // Get data from Antares
  if (millis() - previousMillis2 > interval2) { // Check if it's time to get data from Antares
    previousMillis2 = millis(); // Update the last received time
    antares.get(projectName, deviceNameControl);
    if (antares.getSuccess()) {
      String data = antares.getString("data");
      Serial.println("Data: " + data);
      int durasi = antares.getInt("durasi");
      Serial.println("Durasi: " + durasi);
      if(durasi == NULL){
        if (data == "1OPEN") {
          digitalWrite(RELAY_PIN1, LOW); // Set the relay to LOW
          // digitalWrite(RELAY_PIN2, LOW);
          Serial.println("Relay1 turned LOW");
        } else if (data == "1CLOSE") {
          digitalWrite(RELAY_PIN1, HIGH); // Set the relay to HIGH
          // digitalWrite(RELAY_PIN2, HIGH);
          Serial.println("Relay1 turned HIGH");
          //disarankan untuk telekomunikasi mengirim data "CLOSE" setelah command-command dibawah ini jika tidak, nanti akan loop.
        } else if (data == "2OPEN") {
          // digitalWrite(RELAY_PIN1, LOW);
          digitalWrite(RELAY_PIN2, LOW); // Set the relay to LOW
          Serial.println("Relay2 turned LOW");
        } else if (data == "2CLOSE") {
          // digitalWrite(RELAY_PIN1, HIGH);
          digitalWrite(RELAY_PIN2, HIGH); // Set the relay to HIGH
          Serial.println("Relay2 turned HIGH");
          //disarankan untuk telekomunikasi mengirim data "CLOSE" setelah command-command dibawah ini jika tidak, nanti akan loop.
        } 
      }else{
        if (data == "1OPEN") {
          digitalWrite(RELAY_PIN1, LOW); // Set the relay to LOW
          // digitalWrite(RELAY_PIN2, LOW);
          Serial.println("Relay1 turned LOW");
          Serial.print("Durasi : ");
          Serial.println((durasi * 1000));
          delay((durasi * 1000));
          digitalWrite(RELAY_PIN1, HIGH);
          // digitalWrite(RELAY_PIN2, HIGH);
          Serial.println("Relay1 turned HIGH");
        } else if (data == "1CLOSE") {
          digitalWrite(RELAY_PIN1, HIGH); // Set the relay to HIGH
          // digitalWrite(RELAY_PIN2, HIGH);
          Serial.println("Relay1 turned HIGH");
          //disarankan untuk telekomunikasi mengirim data "CLOSE" setelah command-command dibawah ini jika tidak, nanti akan loop.
        } else if (data == "2OPEN") {
          // digitalWrite(RELAY_PIN1, LOW);
          digitalWrite(RELAY_PIN2, LOW); // Set the relay to LOW
          Serial.println("Relay1 turned LOW");
          Serial.print("Durasi : ");
          Serial.println((durasi * 1000));
          delay((durasi * 1000));
          // digitalWrite(RELAY_PIN1, HIGH);
          digitalWrite(RELAY_PIN2, HIGH);
          Serial.println("Relay2 turned HIGH");
        } else if (data == "2CLOSE") {
          // digitalWrite(RELAY_PIN1, HIGH);
          digitalWrite(RELAY_PIN2, HIGH); // Set the relay to HIGH
          Serial.println("Relay2 turned HIGH");
          //disarankan untuk telekomunikasi mengirim data "CLOSE" setelah command-command dibawah ini jika tidak, nanti akan loop.
        } 
      }
      delay(1000);
    }
  }
}
