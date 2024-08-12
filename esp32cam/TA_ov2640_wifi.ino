#include <esp_now.h>
#include "esp_camera.h"
#include "base64.h"
#include <AntaresESPHTTP.h>

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define fileDatainMessage 240.0
#define UARTWAITHANDSHACK 1000
// Global copy of slave
esp_now_peer_info_t slave;
const int chunkSize = 225;

#define ONBOADLED 4

#define ACCESSKEY "52611abd7352ee93:5b27d91bb8c356e7" // Antares account access key
#define WIFISSID "****" // Wi-Fi SSID
#define PASSWORD "****" // Wi-Fi password
#define projectName "IoTBawangMerah_gen2" // Antares application name 
#define deviceName "data_camera" // Antares device name 
AntaresESPHTTP antares(ACCESSKEY);

void setup() {
  Serial.begin(115200);
  Serial.println("CAMERA MASTER STARTED");

  antares.setDebug(true);
  antares.wifiConnection(WIFISSID, PASSWORD);

  initCamera();
  takephoto();
  delay(5000);
}


void loop() {}

void initCamera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;

  Serial.println("psramFound() = " + String(psramFound()));

  if (psramFound()) {
    config.fb_count = 2;
  } else {
    config.fb_count = 1;
  }

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }else{
    Serial.println("Camera init successfully");
  }
}

void takephoto(){
  camera_fb_t * fb = NULL;
  String base64_image = "";
  delay(50);
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  
  base64_image = base64::encode((uint8_t *)fb->buf, fb->len);
  Serial.println("Gambar dalam format Base64:");
  Serial.println(base64_image);

  int length = base64_image.length();
  int i = 0;
  int j = ceil(length/chunkSize) + 1;
  for (int currentLength = 0; currentLength < length; currentLength += chunkSize) {
    char chunk[230];
    base64_image.substring(currentLength, min(currentLength + chunkSize, length)).toCharArray(chunk, chunkSize + 1);
    
    i += 1;
    antares.add("id", "plant001");
    antares.add("image_data", chunk);
    antares.add("posisi", "atas");
    antares.add("bagian", i);
    if(i == j){
      antares.add("status", "end");
      antares.send(projectName, deviceName);
    }else{
      antares.add("status", "next");
      antares.send(projectName, deviceName);
    }
  }
  esp_camera_fb_return(fb);
}
