#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "esp_camera.h"
#include "time.h"
#include "FS.h"
#include "SPIFFS.h"

// Konfigurasi kamera (ESP32-CAM AI Thinker)
#define PWDN_GPIO_NUM     -1
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

// PIN untuk PIR dan buzzer
#define PIR_PIN 13
#define BUZZER_PIN 12

// Telegram bot token dan chat ID
const char* botToken = "YOUR_BOT_TOKEN";
const char* chatId = "YOUR_CHAT_ID";

// Koordinat lokasi (bisa statis atau dari GPS)
const char* location = "Lat: -6.200000, Lon: 106.816666"; // Contoh Jakarta

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

bool motionDetected = false;
unsigned long lastTrigger = 0;
unsigned long delayBetweenTriggers = 10000; // 10 detik delay antar trigger

// Zona waktu
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;

void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x", err);
    return;
  }
}

void setup() {
  Serial.begin(115200);

  // Setup PIR dan Buzzer
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // WiFi Manager
  WiFiManager wm;
  bool res;
  res = wm.autoConnect("ESP32-CAM-SECURITY");

  if(!res) {
    Serial.println("Failed to connect WiFi.");
    ESP.restart();
  }

  Serial.println("WiFi connected!");

  // Inisialisasi kamera
  setupCamera();

  // Inisialisasi waktu
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Telegram HTTPS secure
  client.setInsecure();
}

String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "N/A";
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

void loop() {
  int motion = digitalRead(PIR_PIN);

  if (motion == HIGH && millis() - lastTrigger > delayBetweenTriggers) {
    Serial.println("Gerakan Terdeteksi!");

    lastTrigger = millis();

    // Aktifkan buzzer
    digitalWrite(BUZZER_PIN, HIGH);
    delay(2000);
    digitalWrite(BUZZER_PIN, LOW);

    // Ambil foto
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Gagal mengambil gambar");
      return;
    }

    // Kirim ke Telegram
    Serial.println("Mengirim ke Telegram...");

    String caption = "📸 *Gerakan Terdeteksi!*\n";
    caption += "🕒 " + getTimestamp() + "\n";
    caption += "📍 " + String(location);

    bot.sendPhotoByBinary(chatId, "image/jpeg", fb->len, fb->buf, "image.jpg", caption);

    esp_camera_fb_return(fb);
  }

  delay(500);
}
