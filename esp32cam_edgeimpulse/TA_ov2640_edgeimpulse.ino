#include "esp_camera.h"
#include "base64.h"
#include <AntaresESPHTTP.h>
#include <TA_bawang_merah_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"

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

const int chunkSize = 450;

/* Constant defines -------------------------------------------------------- */
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS           320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS           240
#define EI_CAMERA_FRAME_BYTE_SIZE                 3

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static bool is_initialised = false;
uint8_t *snapshot_buf; //points to the output of the capture


String base64_image = "";
String category = "";

#define ONBOADLED 4

#define ACCESSKEY "52611abd7352ee93:5b27d91bb8c356e7" // Antares account access key
#define WIFISSID "E5576_AFF2" // Wi-Fi SSID
#define PASSWORD "25LndEd2G97" // Wi-Fi password
#define projectName "IoTBawangMerah_gen2" // Antares application name that was created
#define deviceName "data_camera" // Antares device name that was created
AntaresESPHTTP antares(ACCESSKEY);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("CAMERA MASTER STARTED");

  antares.setDebug(true);
  antares.wifiConnection(WIFISSID, PASSWORD);

  initCamera();
  // takephoto();
  delay(60000);
}



void loop() {
  // delay 5 menit
  // delay(7000);
  
  category = "";
  snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);

    // check if allocation was successful
    if(snapshot_buf == nullptr) {
        ei_printf("ERR: Failed to allocate snapshot buffer!\n");
        return;
    }

    if (ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf) == false) {
        ei_printf("Failed to capture image\r\n");
        free(snapshot_buf);
        return;
    }

    String category = printResult();
    
    if(category == ""){
        ei_printf("Failed to predict..\r\n");
        return;
    }

    sendDataAntares();

    base64_image = "";
    category = "";

    free(snapshot_buf);

    // Konfigurasi untuk bangun dari deep sleep setelah 1 hari (24 jam)
    esp_sleep_enable_timer_wakeup(24 * 60 * 60 * 1000000ULL); // 1 hari dalam mikrodetik
    // Masuk ke deep sleep
    esp_deep_sleep_start();
}
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
  // config.pixel_format = PIXFORMAT_YUV422;
  // config.pixel_format = PIXFORMAT_GRAYSCALE;
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
    is_initialised = true;
    Serial.println("Camera init successfully");
  }
}

bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
    bool do_resize = false;

    if (!is_initialised) {
        ei_printf("ERR: Camera is not initialized\r\n");
        return false;
    }

    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb) {
        ei_printf("Camera capture failed\n");
        return false;
    }
    
    base64_image = base64::encode((uint8_t *)fb->buf, fb->len);
    Serial.println("Gambar dalam format Base64:");
    Serial.println(base64_image);

   bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);

   esp_camera_fb_return(fb);

   if(!converted){
       ei_printf("Conversion failed\n");
       return false;
   }

    if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS)
        || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
        do_resize = true;
    }

    if (do_resize) {
        ei::image::processing::crop_and_interpolate_rgb888(
        out_buf,
        EI_CAMERA_RAW_FRAME_BUFFER_COLS,
        EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
        out_buf,
        img_width,
        img_height);
    }


    return true;
}

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr)
{
    // we already have a RGB888 buffer, so recalculate offset into pixel index
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;

    while (pixels_left != 0) {
        // Swap BGR to RGB here
        // due to https://github.com/espressif/esp32-camera/issues/379
        out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix];

        // go to the next pixel
        out_ptr_ix++;
        pixel_ix+=3;
        pixels_left--;
    }
    // and done!
    return 0;
}

String printResult()
{
  // Run the classifier
    ei_impulse_result_t result = { 0 };
    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", err);
        return "";
    }

    // print the predictions
    ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
                result.timing.dsp, result.timing.classification, result.timing.anomaly);

    ei_printf("Predictions:\r\n");
    float value = 0;
    String pred = "";
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if(value < result.classification[i].value){
          value = result.classification[i].value;
          pred = ei_classifier_inferencing_categories[i];
        }
        ei_printf("  %s: ", ei_classifier_inferencing_categories[i]);
        ei_printf("%.5f\r\n", result.classification[i].value);
    }

    if(pred == ""){
      free(snapshot_buf);
      ei_printf("failed to predict..\r\n");
      return "";
    }else{
      Serial.print("prediksi:");
      Serial.println(pred);
      Serial.println();
    }

    return pred;
}

void sendDataAntares()
{
   int length = base64_image.length();
   int i = 0;
   int j = ceil(length/chunkSize) + 1;
   for (int currentLength = 0; currentLength < length; currentLength += chunkSize) {
     char chunk[460];
     base64_image.substring(currentLength, min(currentLength + chunkSize, length)).toCharArray(chunk, chunkSize + 1);
     
     i += 1;
     antares.add("id", "plant003");
     antares.add("image_data", chunk);
     antares.add("posisi", "atas");
     antares.add("bagian", i);
     antares.add("prediksi", category);
     if(i == j){
       antares.add("status", "end");
       antares.send(projectName, deviceName);
     }else{
       antares.add("status", "next");
       antares.send(projectName, deviceName);
     }
   }
}
