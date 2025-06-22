#include <LiquidCrystal.h>
#define BLYNK_TEMPLATE_ID "TMPL6tEq3QsY5"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "GNz9xY3fdbv-qQeydQWS0x_RwWRlsn_v"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

char ssid[] = "tebakberhadiah";
char pass[] = "02062005";

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define SENSOR_SUARA 35
#define RELAY_PIN 2
#define LED_TEPUK_PIN 13 
#define LDR_PIN 34
#define LED_MALAM_PIN 16 

int clap = 0;
long detection_range_start = 0;
long detection_range = 0;
boolean status_lampu_tepuk = false;
int ambangBatasGelap = 2500;

BlynkTimer timer;
unsigned long lastLcdUpdateTime = 0;
const long lcdUpdateInterval = 500;

// Tambahan variabel baru
bool modeOtomatisLedMalam = true;  // true = otomatis, false = manual
bool statusLedMalamManual = true;  // HIGH = OFF (sesuai wiring)

// Tombol kontrol mode otomatis/manual di V4
BLYNK_WRITE(V6) {
  modeOtomatisLedMalam = param.asInt();
}

// Tombol kontrol manual LED malam di V5
BLYNK_WRITE(V5) {
  statusLedMalamManual = param.asInt();
  if (!modeOtomatisLedMalam) {
    digitalWrite(LED_MALAM_PIN, statusLedMalamManual ? LOW : HIGH);
    Blynk.virtualWrite(V2, statusLedMalamManual ? "ON" : "OFF");
  }
}

// Tombol kontrol LED tepuk dari Blynk di V0
BLYNK_WRITE(V0) {
  int pinValue = param.asInt(); 
  if (pinValue == 1) {
    status_lampu_tepuk = true;
    digitalWrite(RELAY_PIN, LOW); 
    digitalWrite(LED_TEPUK_PIN, HIGH);
    Blynk.virtualWrite(V1, "OFF"); 
  } else {
    status_lampu_tepuk = false;
    digitalWrite(RELAY_PIN, HIGH); 
    digitalWrite(LED_TEPUK_PIN, LOW);
    Blynk.virtualWrite(V1, "ON");
  }
}

void sendSensorData() {
  int nilaiCahaya = analogRead(LDR_PIN);
  Blynk.virtualWrite(V3, nilaiCahaya);
}

void updateLcdDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("Te:");
  lcd.print(status_lampu_tepuk ? "OFF" : "ON ");
  lcd.setCursor(9, 0);
  lcd.print("Ma:");

  if (digitalRead(LED_MALAM_PIN) == LOW)
    lcd.print("ON ");
  else
    lcd.print("OFF");

  lcd.setCursor(0, 1);
  int nilaiCahaya = analogRead(LDR_PIN);
  lcd.print("LDR:");
  lcd.print(nilaiCahaya);
  lcd.print("    ");
}

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Room");
  lcd.setCursor(0, 1);
  lcd.print("Menyambungkan...");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  lcd.clear();
  timer.setInterval(1000L, sendSensorData);

  pinMode(SENSOR_SUARA, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_TEPUK_PIN, OUTPUT);
  pinMode(LED_MALAM_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_TEPUK_PIN, LOW);
  digitalWrite(LED_MALAM_PIN, HIGH); // Awal OFF
}

void loop() {
  Blynk.run(); 
  timer.run(); 

  if (millis() - lastLcdUpdateTime > lcdUpdateInterval) {
    lastLcdUpdateTime = millis();
    updateLcdDisplay();
  }

  // Saklar tepuk
  int status_sensor = digitalRead(SENSOR_SUARA);
  if (status_sensor == 0) {
    if (clap == 0) {
      detection_range_start = detection_range = millis();
      clap++;
    } else if (clap > 0 && millis() - detection_range >= 50) {
      detection_range = millis();
      clap++;
    }
  }
  if (millis() - detection_range_start >= 400) {
    if (clap == 2) {
      status_lampu_tepuk = !status_lampu_tepuk; 
      if (status_lampu_tepuk) {
        digitalWrite(LED_TEPUK_PIN, HIGH);
        digitalWrite(RELAY_PIN, LOW);
        Blynk.virtualWrite(V1, "OFF");
        Blynk.virtualWrite(V0, 1);
      } else {
        digitalWrite(LED_TEPUK_PIN, LOW);
        digitalWrite(RELAY_PIN, HIGH);
        Blynk.virtualWrite(V1, "ON");
        Blynk.virtualWrite(V0, 0);
      }
    }
    clap = 0;
  }

  // Logika LED malam
  if (modeOtomatisLedMalam) {
    int nilaiCahaya = analogRead(LDR_PIN);
    if (nilaiCahaya < ambangBatasGelap) {
      digitalWrite(LED_MALAM_PIN, LOW);
      Blynk.virtualWrite(V2, "ON");
    } else {
      digitalWrite(LED_MALAM_PIN, HIGH);
      Blynk.virtualWrite(V2, "OFF");
    }
  }
}
