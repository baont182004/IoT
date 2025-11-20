/******************************************************
 *  ESP8266 + MQ2 + Blynk + Backend Cloud (MongoDB)
 ******************************************************/

#define BLYNK_TEMPLATE_ID   "TMPL6FoIRsM96"
#define BLYNK_TEMPLATE_NAME "KHIGAS"
#define BLYNK_AUTH_TOKEN    "T3fpZKQNeKv7JAgVGg3GGmt9ebSiJ_5m" 

#define DEBUG
#include "espConfig.h" // State machine cấu hình WiFi + Blynk

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

/***************** KHAI BÁO PHẦN CẢNH BÁO + BLYNK *****************/

int buzzer = 5;      // D1
int relay  = 4;      // D2
int ledMode = 14;    // D5
int mucCanhbao;
BlynkTimer timer;
int timerID1, timerID2;
float mq2_value;
int button = 0;      // D3
boolean buttonState = HIGH;
boolean runMode = 0;       // Bật/tắt chế độ cảnh báo
boolean canhbaoState = 0;
WidgetLED led(V0);         // LED trạng thái trên Blynk

#define GAS         V1
#define MUCCANHBAO  V2
#define CANHBAO     V3
#define CHEDO       V4

/***************** KHAI BÁO PHẦN BACKEND CLOUD *****************/

// Device ID & Backend URL (Render)
const char* DEVICE_ID   = "esp8266-gas-01";
const char* BACKEND_URL = "https://bilstm-iot.onrender.com/api/gas";


/******************************************************
 *  HÀM GỬI DỮ LIỆU LÊN BACKEND
 ******************************************************/
void sendGasToBackend(float gasValue, int rawAdc) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[HTTP] WiFi chưa kết nối, bỏ qua"));
    return;
  }

  WiFiClientSecure client;
  client.setInsecure(); 

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


/***************** VÒNG ĐỜI CHƯƠNG TRÌNH *****************/

void setup() {
  Serial.begin(115200);

  pinMode(button, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  pinMode(relay, OUTPUT);
  pinMode(ledMode, OUTPUT);

  digitalWrite(buzzer, LOW); // Tắt buzzer
  digitalWrite(relay, LOW);

  // Đọc cảm biến, cập nhật Blynk & backend mỗi 1 giây
  timerID1 = timer.setInterval(1000L, handleTimerID1);

  // Khởi động hệ thống cấu hình WiFi + Blynk
  espConfig.begin();
}

void app_loop() {
  timer.run();

  // Xử lý nút nhấn chuyển chế độ runMode
  if (digitalRead(button) == LOW) {
    if (buttonState == HIGH) {
      buttonState = LOW;
      runMode = !runMode;
      digitalWrite(ledMode, runMode);
      Serial.println("Run mode: " + String(runMode));
      Blynk.virtualWrite(CHEDO, runMode);
      delay(200);
    }
  } else {
    buttonState = HIGH;
  }
}

void loop() {
  espConfig.run();  // Quản lý WiFi + Blynk
  app_loop();       // Chạy timer & xử lý nút
}


/***************** XỬ LÝ TIMER ĐỌC MQ2 + CẢNH BÁO *****************/

void handleTimerID1() {
  // Đọc MQ2
  int mq2 = analogRead(A0);
  float voltage = mq2 / 1024.0 * 5.0;
  float ratio = voltage / 1.4;
  mq2_value = 1000.0 * pow(10, ((log10(ratio) - 1.0278) / 0.6629));

  Serial.println("Gas: " + String(mq2_value, 0) + "ppm");

  // Cập nhật lên Blynk (giữ nguyên logic cũ)
  Blynk.virtualWrite(GAS, mq2_value);

  if (led.getValue()) {
    led.off();
  } else {
    led.on();
  }

  // Logic cảnh báo cũ
  if (runMode == 1) {
    if (mq2_value > mucCanhbao) {
      if (canhbaoState == 0) {
        canhbaoState = 1;
        Blynk.logEvent("canhbao",
          String("Cảnh báo! Khí gas=" + String(mq2_value) + " vượt quá mức cho phép!"));
        timerID2 = timer.setTimeout(60000L, handleTimerID2);
      }
      digitalWrite(buzzer, HIGH);
      digitalWrite(relay, HIGH);
      Blynk.virtualWrite(CANHBAO, HIGH);
      Serial.println("Đã bật cảnh báo!");
    } else {
      digitalWrite(buzzer, LOW);
      digitalWrite(relay, LOW);
      Blynk.virtualWrite(CANHBAO, LOW);
      Serial.println("Đã tắt cảnh báo!");
    }
  } else {
    digitalWrite(buzzer, LOW);
    digitalWrite(relay, LOW);
    Blynk.virtualWrite(CANHBAO, LOW);
    Serial.println("Đã tắt cảnh báo!");
  }

  // *** GỬI DỮ LIỆU LÊN BACKEND ***
  int rawAdc = mq2;                 // raw ADC từ A0
  sendGasToBackend(mq2_value, rawAdc);
}

void handleTimerID2() {
  canhbaoState = 0;
}


/***************** BLYNK HANDLERS *****************/

BLYNK_CONNECTED() {
  Blynk.syncAll();
}

BLYNK_WRITE(MUCCANHBAO) {
  mucCanhbao = param.asInt();
}

BLYNK_WRITE(CHEDO) {
  runMode = param.asInt();
  digitalWrite(ledMode, runMode);
}
