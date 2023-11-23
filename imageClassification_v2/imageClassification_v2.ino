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

const char *ssid = "***";
const char *pass = "***";
const unsigned long timeout = 30000;  // 30 seconds

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
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Settings
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  // Initializing camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    digitalWrite(LEDR, HIGH);
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Video options
  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 1);       //1-Upside down, 0-No operation
  s->set_hmirror(s, 1);     //1-Reverse left and right, 0-No operation
  s->set_brightness(s, 2);  //up the blightness just a bit
  // s->set_saturation(s, -1);  //lower the saturation

  // drop down frame size for higher initial frame rate
  // s->set_framesize(s, FRAMESIZE_VGA);
  Serial.println("Device ON!");
}

void postImage(camera_fb_t *fb) {
  HTTPClient http;
  // Add server resource
  http.begin("https://esp32-api-mediator-36025d758c37.herokuapp.com:55925/imageObjects");
  // imageObjects or imageLabels
  http.addHeader("Content-Type", "image/jpeg");
  int httpResponseCode = http.POST(fb->buf, fb->len);

  // Check response code
  if (httpResponseCode == 200) {
    String response = http.getString();
    parseResults(response);
  } else {
    // show error
  }

  // Close connection
  http.end();
  WiFi.disconnect();
}

void parseResults(String response) {
  // Calculate correct size later
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, response);
  JsonArray array = doc.as<JsonArray>();
  for (JsonVariant v : array) {
    JsonObject object = v.as<JsonObject>();
    //do something
    const char *name = object["name"];
    float score = object["score"];
    Serial.println(name);
    Serial.println(score);
    Serial.println();
  }
}

void sendImage() {
  camera_fb_t *fb = capture();
  if (!fb || fb->format != PIXFORMAT_JPEG) {
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);
    return;
  } else {
    if (wifiConnect()) {
      // send image to server
      // add led/lcd indication
      postImage(fb);
    } else {
      // Show error
      Serial.println("Error sending image");
    }
    esp_camera_fb_return(fb);
  }
}

camera_fb_t *capture() {
  // Create image using camera
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  fb = esp_camera_fb_get();
  return fb;
}

bool wifiConnect() {
  unsigned long startingTime = millis();
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if ((millis() - startingTime) > timeout) {
      digitalWrite(LEDR, HIGH);
      return false;
    }
  }
  return true;
}

void loop() {

  if( digitalRead(BUTTON) == HIGH ){
    digitalWrite(LEDG, HIGH);
    sendImage();
    digitalWrite(LEDG, LOW);
  }
}