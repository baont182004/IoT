#define BLYNK_TEMPLATE_ID   "TMPL6FoIRsM96"
#define BLYNK_TEMPLATE_NAME "KHIGAS"
#define BLYNK_AUTH_TOKEN    "T3fpZKQNeKv7JAgVGg3GGmt9ebSiJ_5m" // Đã thêm Auth Token

#define DEBUG
#include "espConfig.h" // Thư viện cấu hình của bạn
int buzzer=5;     //D1
int relay =4;     //D2
int ledMode=14;   //D5
int mucCanhbao;
BlynkTimer timer;
int timerID1,timerID2;
float mq2_value;
int button=0;     //D3
boolean buttonState=HIGH;
boolean runMode=0;//Bật/tắt chế độ cảnh báo
boolean canhbaoState=0;
WidgetLED led(V0);
#define GAS V1
#define MUCCANHBAO V2
#define CANHBAO V3
#define CHEDO V4

void setup() {
  Serial.begin(115200);
  pinMode(button,INPUT_PULLUP);
  pinMode(buzzer,OUTPUT);
  pinMode(relay,OUTPUT);
  pinMode(ledMode,OUTPUT);
  digitalWrite(buzzer,LOW); //Tắt buzzer
  digitalWrite(relay,LOW);
  timerID1 = timer.setInterval(1000L,handleTimerID1);
  espConfig.begin();
}

void loop() {
  espConfig.run();
  app_loop();
}
void app_loop(){
  timer.run();
  if(digitalRead(button)==LOW){
    if(buttonState==HIGH){
      buttonState=LOW;
      runMode=!runMode;
      digitalWrite(ledMode,runMode);
      Serial.println("Run mode: " + String(runMode));
      Blynk.virtualWrite(CHEDO,runMode);
      delay(200);
    }
  }else{
    buttonState=HIGH;
  }
}
void handleTimerID1(){
  int mq2 = analogRead(A0);
  float voltage = mq2 / 1024.0 * 5.0;
  float ratio = voltage / 1.4;
  mq2_value = 1000.0 * pow(10, ((log10(ratio) - 1.0278) / 0.6629));
  Serial.println("Gas: "+String(mq2_value,0)+"ppm");
  Blynk.virtualWrite(GAS,mq2_value);
  if(led.getValue()) {
    led.off();
  } else {
    led.on();
  }
  if(runMode==1){
    if(mq2_value>mucCanhbao){
      if(canhbaoState==0){
        canhbaoState=1;
        Blynk.logEvent("canhbao", String("Cảnh báo! Khí gas=" + String(mq2_value)+" vượt quá mức cho phép!"));
        timerID2 = timer.setTimeout(60000L,handleTimerID2);
      }
      digitalWrite(buzzer,HIGH);
      digitalWrite(relay,HIGH);
      Blynk.virtualWrite(CANHBAO,HIGH);
      Serial.println("Đã bật cảnh báo!");
    }else{
      digitalWrite(buzzer,LOW);
      digitalWrite(relay,LOW);
      Blynk.virtualWrite(CANHBAO,LOW);
      Serial.println("Đã tắt cảnh báo!");
    }
  }else{
    digitalWrite(buzzer,LOW);
    digitalWrite(relay,LOW);
    Blynk.virtualWrite(CANHBAO,LOW);
    Serial.println("Đã tắt cảnh báo!");
  }
}
void handleTimerID2(){
  canhbaoState=0;
}
BLYNK_CONNECTED() {
  Blynk.syncAll();
}
BLYNK_WRITE(MUCCANHBAO) {
  mucCanhbao = param.asInt();
}
BLYNK_WRITE(CHEDO) {
  runMode = param.asInt();
  digitalWrite(ledMode,runMode);
}