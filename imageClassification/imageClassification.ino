#include <WiFi.h>
#include <HTTPClient.h>
//#include <Arduino.h>
#include <ArduinoJson.h>
#include "esp_camera.h"
#include <base64.h>

// #define CAMERA_MODEL_WROVER_KIT

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

void startCameraServer();

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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  // config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition

  // config.grab_mode = CAMERA_GRAB_WHEN_EMPTY; // I dont know what this does
  // config.fb_location = CAMERA_FB_IN_PSRAM; // Nor this

  // Setting 1
  // config.frame_size = FRAMESIZE_UXGA;
  // config.jpeg_quality = 10;
  // config.fb_count = 2;

  // Setting 2
  config.frame_size = FRAMESIZE_CIF;
  config.jpeg_quality = 10; // Higher value more fps, I think, this was 12 originally
  config.fb_count = 1;

  // Initializing camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    digitalWrite(LEDR, HIGH);
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Video options
  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 1);        //1-Upside down, 0-No operation
  s->set_hmirror(s, 1);      //1-Reverse left and right, 0-No operation
  s->set_brightness(s, 2);   //up the blightness just a bit
  // s->set_saturation(s, -1);  //lower the saturation

  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_VGA);

  // WiFi Setup
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LEDR, HIGH);
    delay(500);
    digitalWrite(LEDR, LOW);
    delay(500);
  }

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  startCameraServer();

}

void loop() {

  if (digitalRead(BUTTON) == HIGH) {
    classify();
    //Serial.println(digitalRead(BUTTON));
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
  size_t size = fb->len;
  String buffer = base64::encode((uint8_t *)fb->buf, fb->len);
  String payload = "{\"user_app_id\": {\"user_id\": \"8rmb75748zzm\",\"app_id\": \"ESP32-API\"},\"inputs\": [{ \"data\": {\"image\": {\"base64\": \"" + buffer + "\"}}}]}";

  buffer = "";
  // Uncomment this if you want to show the payload
  //Serial.println(payload);

  esp_camera_fb_return(fb);

  // API Setup
  String model_id = "general-image-recognition";

  HTTPClient http;
  http.begin("https://api.clarifai.com/v2/models/" + model_id + "/outputs");
  http.addHeader("Accept", "application/json");
  http.addHeader("Authorization", "Key ***");

  int response_code = http.POST(payload);
  String response;

  if (response_code > 0) {
    digitalWrite(LEDG, HIGH);
    Serial.print(response_code);
    Serial.print("Returned String: ");
    response = http.getString();
    Serial.println(response);
    digitalWrite(LEDG, LOW);
  } else {
    Serial.print("POST Error: ");
    Serial.print(response_code);
    return;
  }

  const int jsonSize = 2 * JSON_ARRAY_SIZE(0) + JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(20) + 4 * JSON_OBJECT_SIZE(0) + 7 * JSON_OBJECT_SIZE(1) + 5 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 21 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(7) + JSON_OBJECT_SIZE(18) + 3251;
  DynamicJsonDocument doc(jsonSize);
  deserializeJson(doc, response);

  for (int i = 0; i < 10; i++) {

    const String name = doc["outputs"][0]["data"]["concepts"][i]["name"];
    const float prob = doc["outputs"][0]["data"]["concepts"][i]["value"];

    Serial.println("________________________");
    Serial.print("Name:");
    Serial.println(name);
    Serial.print("Probability:");
    Serial.println(prob);
    Serial.println();
  }
}