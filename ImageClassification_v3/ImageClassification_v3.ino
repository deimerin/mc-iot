#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include "esp_camera.h"
#include <UltrasonicSensor.h>

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

#define LED 2
#define SERVOPIN 15
#define BUZZERPIN 0

UltrasonicSensor lidSensor(33, 32);
UltrasonicSensor baseSensor(13, 14);

Servo lidServo;

const char *ssid = "****";
const char *pass = "****";

String apiKeyIn = "*****";  

void setup() {
  Serial.begin(9600);

  // Connected Pins
  pinMode(LED, OUTPUT);
  pinMode(BUZZERPIN, OUTPUT);

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
  config.xclk_freq_hz = 8000000;         // 20000000 default
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming

  config.grab_mode = CAMERA_GRAB_LATEST;  // I dont know what this does

  // Setting 1
  config.frame_size = FRAMESIZE_CIF;  // CIF works fine, but I might try something else
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // Initializing camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    digitalWrite(LED, HIGH);
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // This can help picture quality

  // Video options
  // sensor_t *s = esp_camera_sensor_get();
  // s->set_vflip(s, 1);       //1-Upside down, 0-No operation
  // s->set_hmirror(s, 1);     //1-Reverse left and right, 0-No operation
  // s->set_brightness(s, 2);  //up the blightness just a bit
  // s->set_saturation(s, -1);  //lower the saturation

  // WiFi Setup
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    delay(500);
  }

  lidServo.setPeriodHertz(50);
  lidServo.attach(SERVOPIN, 500, 2500);

  int temperature = 22;
  lidSensor.setTemperature(temperature);
  baseSensor.setTemperature(temperature);

  powerOnSound();
  Serial.println("Device ON!");
}

void loop() {

  int baseDistance = baseSensor.distanceInCentimeters();
  // first test indicate a range between 20 to 25 cms
  if (baseDistance > 20 && baseDistance < 25) {
    Serial.println(baseDistance);
    okSound();
    classify();
  }
  delay(500);
}

void openCloseLid(int del) {
  lidOpeningSound();
  lidServo.write(35);
  delay(150);
  lidServo.write(150);

  delay(del);

  lidClosingSound();
  lidServo.write(35);
  delay(1000);
  updateSensor();
}

void classify() {

  // Create an image using the camera
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  esp_camera_fb_return(fb);
  fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Camera capture failed");
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    return;
  }

  // API Setup
  HTTPClient http;
  http.begin("http://esp32-api-mediator-36025d758c37.herokuapp.com/upload");
  http.addHeader("Content-Type", "application/octet-stream");
  int httpResponseCode = http.POST(fb->buf, fb->len);

  Serial.println(httpResponseCode);
  esp_camera_fb_return(fb);

  if (httpResponseCode > 0) {
    String jsonResponse = http.getString();
    Serial.println("Received JSON response:");

    if (isObjectValid(jsonResponse)) {
      Serial.println("VALID");
      openCloseLid(5000);
    } else {
      Serial.println("NOT VALID");
      errorSound();
    }
  } else {
    Serial.print("HTTP request failed with error code: ");
    Serial.println(httpResponseCode);
    errorSound();
  }

  http.end();
}

bool isObjectValid(const String &jsonString) {
  const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_ARRAY_SIZE(10) + 60;
  DynamicJsonDocument doc(capacity);

  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) {
    Serial.print(F("Error parsing JSON: "));
    Serial.println(error.c_str());
    errorSound();
    return false;
  }

  // Check if the 'filteredData' array exists
  if (doc.containsKey("filteredData")) {
    JsonArray filteredData = doc["filteredData"];
    Serial.println(filteredData.size());
    return filteredData.size() > 0;
  }

  return false;
}

void powerOnSound() {

  for (int i = 0; i < 3; ++i) {
    digitalWrite(BUZZERPIN, HIGH);
    delay(100);
    digitalWrite(BUZZERPIN, LOW);
    delay(50);
  }

  delay(200);
  digitalWrite(BUZZERPIN, LOW);
}

void errorSound() {

  for (int i = 0; i < 5; ++i) {
    digitalWrite(BUZZERPIN, HIGH);
    delay(200);
    digitalWrite(BUZZERPIN, LOW);
    delay(100);
  }

  digitalWrite(BUZZERPIN, LOW);
}

void okSound() {

  for (int i = 0; i < 2; ++i) {
    digitalWrite(BUZZERPIN, HIGH);
    delay(150);
    digitalWrite(BUZZERPIN, LOW);
    delay(50);
  }

  digitalWrite(BUZZERPIN, LOW);
}

void lidOpeningSound() {

  for (int i = 0; i < 2; ++i) {
    digitalWrite(BUZZERPIN, HIGH);
    delay(150);
    digitalWrite(BUZZERPIN, LOW);
    delay(50);
  }

  digitalWrite(BUZZERPIN, LOW);
}

void lidClosingSound() {

  for (int i = 0; i < 3; ++i) {
    digitalWrite(BUZZERPIN, HIGH);
    delay(200);
    digitalWrite(BUZZERPIN, LOW);
    delay(50);
  }

  digitalWrite(BUZZERPIN, LOW);
}

void updateSensor() {
  HTTPClient ask;
  int lidDistance = lidSensor.distanceInCentimeters();
  int percentageDistance = map(lidDistance, 2, 17, 100, 0);
  Serial.println(lidDistance);
  Serial.println(percentageDistance);
  String url = "http://api.asksensors.com/write/" + apiKeyIn + "?module1=" + String(percentageDistance);

  ask.begin(url);
  int httpCode = ask.GET();

  if (httpCode > 0) {
    Serial.println(ask.getString());
  } else {
    Serial.println("Error on HTTP request");
    Serial.println(ask.errorToString(httpCode).c_str());
  }

  ask.end();
}