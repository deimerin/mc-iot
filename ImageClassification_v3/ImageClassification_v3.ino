#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "esp_camera.h"

#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 21
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 19
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 5
#define Y2_GPIO_NUM 4
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define LEDR 13
#define LEDG 0
#define BUTTON 32

const char *ssid = "CARDENAS";
const char *pass = "UqdkN296";

void setup() {
  Serial.begin(9600);

  // Connected Pins
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(BUTTON, INPUT);

  // Camera config
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
  config.xclk_freq_hz = 8000000;        // 20000000 default
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  // config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition

  // config.grab_mode = CAMERA_GRAB_WHEN_EMPTY; // I dont know what this does
  // config.fb_location = CAMERA_FB_IN_PSRAM; // Nor this

  // Setting 1
  config.frame_size = FRAMESIZE_CIF;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // Initializing camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    digitalWrite(LEDR, HIGH);
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // WiFi Setup
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LEDR, HIGH);
    delay(500);
    digitalWrite(LEDR, LOW);
    delay(500);
  }

  Serial.println("POGGERS");
}

void loop() {

  if (digitalRead(BUTTON) == HIGH) {
    digitalWrite(LEDG, HIGH);
    classify();
    digitalWrite(LEDG, LOW);
  }
}

void classify() {

  // Create an image using the camera
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Camera capture failed");
    digitalWrite(LEDR, HIGH);
    delay(500);
    digitalWrite(LEDR, LOW);
    return;
  }

  // Encoding the image
  esp_camera_fb_return(fb);

  // API Setup
  HTTPClient http;
  http.begin("http://192.168.1.84:8888/imageLabels");
  http.addHeader("Content-Type", "image/jpeg");
  int httpResponseCode = http.POST(fb->buf, fb->len);
}