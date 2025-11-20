/******************************************************
 *  ESP8266 + MQ2 + Blynk + Backend Cloud (MongoDB)
 *  Giữ nguyên toàn bộ cấu trúc espConfig.h & configForm.h
 *  CHỈ thêm phần gửi HTTP POST lên backend Render
 ******************************************************/

#define BLYNK_TEMPLATE_ID   "TMPL6FoIRsM96"
#define BLYNK_TEMPLATE_NAME "KHIGAS"
#define BLYNK_AUTH_TOKEN    "T3fpZKQNeKv7JAgVGg3GGmt9ebSiJ_5m"

#define DEBUG
#include "espConfig.h"   // file cũ của bạn – KHÔNG ĐỤNG TỚI

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// Chân cảm biến MQ2
const int MQ2_PIN = A0;

// Device ID & Backend URL (Render)
const char* DEVICE_ID   = "esp8266-gas-01";
const char* BACKEND_URL = "https://bilstm-iot.onrender.com/api/gas";

// Timer xử lý nhiệm vụ (BlynkTimer)
BlynkTimer timer;


/******************************************************
 *  HÀM GỬI DỮ LIỆU LÊN BACKEND
 ******************************************************/
void sendGasToBackend(float gasValue, int rawAdc) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[HTTP] WiFi chưa kết nối, bỏ qua"));
    return;
  }

  WiFiClientSecure client;
  client.setInsecure(); // cho phép HTTPS không kiểm tra chứng chỉ

  HTTPClient http;

  Serial.println(F("[HTTP] Gửi data lên backend..."));

  if (!http.begin(client, BACKEND_URL)) {
    Serial.println(F("[HTTP] http.begin() FAILED"));
    return;
  }

  http.addHeader("Content-Type", "application/json");

  // JSON payload
  String payload = "{";
  payload += "\"deviceId\":\""; payload += DEVICE_ID;  payload += "\",";
  payload += "\"gasValue\":";   payload += String(gasValue, 2); payload += ",";
  payload += "\"rawAdc\":";     payload += String(rawAdc);
  payload += "}";

  Serial.print(F("[HTTP] Payload: "));
  Serial.println(payload);

  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.printf("[HTTP] Code: %d\n", httpCode);
    if (httpCode >= 200 && httpCode < 300) {
      Serial.println(http.getString());
    }
  } else {
    Serial.print(F("[HTTP] POST FAILED: "));
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}


/******************************************************
 *  ĐỌC CẢM BIẾN MQ2
 ******************************************************/
void readGasTask() {
  int rawAdc = analogRead(MQ2_PIN);           // 0-1023
  float gasValue = (rawAdc / 1023.0) * 1000;  // scale về 0–1000

  Serial.print(F("[SENSOR] raw="));
  Serial.print(rawAdc);
  Serial.print(F(" | gas="));
  Serial.println(gasValue);

  // Gửi lên Blynk
  Blynk.virtualWrite(V0, gasValue);
  Blynk.virtualWrite(V1, rawAdc);

  // Gửi lên backend
  sendGasToBackend(gasValue, rawAdc);
}


/******************************************************
 *  BẮT ĐẦU CHƯƠNG TRÌNH
 ******************************************************/
extern "C" void app_loop() { timer.run(); }

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(MQ2_PIN, INPUT);

  Serial.println();
  Serial.println(F("=== ESP8266 MQ2 Gas Monitor Start ==="));

  // Khởi động bộ quản lý WiFi/Blynk (file espConfig.h)
  espConfig.begin();

  // Đọc & gửi dữ liệu mỗi 5 giây
  timer.setInterval(5000L, readGasTask);
}

void loop() {
  espConfig.run(); // giữ nguyên logic cũ 
  app_loop();      // để timer hoạt động
}
