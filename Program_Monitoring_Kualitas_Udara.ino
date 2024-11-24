#define BLYNK_TEMPLATE_ID "TMPL61Prkei7U"    // Template ID Blynk
#define BLYNK_TEMPLATE_NAME "Aruna"          // Nama Template
#define BLYNK_AUTH_TOKEN "ppTk-lke1BD6g5ZSy3DWpb0yc5H_3E8J"  // Auth Token

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <UniversalTelegramBot.h>
#include <Adafruit_Sensor.h>

#define DHTPIN D1          // Pin DHT11
#define DHTTYPE DHT11      // Tipe sensor DHT11
#define MQ135_PIN A0       // Pin analog untuk MQ135
#define LED_HIJAU D5       // Pin LED hijau
#define LED_MERAH D4       // Pin LED merah
#define BUZZER_PIN D2      // Pin Buzzer

// Informasi Wi-Fi
const char* ssid = "Arunations";
const char* password = "TestWifi30";

// Informasi Telegram
const char* telegramToken = "6470706945:AAGfRs87HNz3YnuiWyFbFPfoGmSIxjsZCh0";
const char* chatIds[] = {"1634461546", "6298395046"}; // Tambahkan ID chat di sini
const int chatIdCount = sizeof(chatIds) / sizeof(chatIds[0]);    // Menghitung jumlah chat ID

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
WiFiClientSecure client;
UniversalTelegramBot bot(telegramToken, client);

const int AQ_THRESHOLD = 150;  // Ambang batas kualitas udara
bool isNotified = false;       // Status apakah notifikasi sudah dikirim

void setup() {
  Serial.begin(9600);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  dht.begin();
  
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  client.setInsecure();  // Menghindari sertifikat
  
  // Mengirim pesan online ke semua ID Telegram
  for (int i = 0; i < chatIdCount; i++) {
    bot.sendMessage(chatIds[i], "ESP8266 ONLINE");
  }

  // Mengatur timer untuk pembaruan berkala
  timer.setInterval(60000L, sendData);  // Mengirim data setiap 1 Menit
}

void sendData() {
  // Membaca nilai dari sensor MQ135 dan DHT
  int mq135Value = analogRead(MQ135_PIN);
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Periksa apakah pembacaan DHT berhasil
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Tampilkan data di Serial Monitor
  Serial.print("Temp: "); Serial.print(t);
  Serial.print(" °C, Humidity: "); Serial.print(h);
  Serial.print(" %, Air Quality: "); Serial.println(mq135Value);

  // Kirim data ke Blynk
  Blynk.virtualWrite(V1, t);            // Suhu
  Blynk.virtualWrite(V2, h);            // Kelembapan
  Blynk.virtualWrite(V3, mq135Value);   // Kualitas udara

  // Periksa kualitas udara dan kelembapan
  bool kondisiNormal = (mq135Value < AQ_THRESHOLD && h >= 45 && h <= 65);
  
  if (kondisiNormal) {
    // Jika kondisi normal, nyalakan LED hijau dan matikan LED merah serta alarm
    digitalWrite(LED_HIJAU, HIGH);
    digitalWrite(LED_MERAH, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Blynk.virtualWrite(V4, 255);        // LED hijau di Blynk
    Blynk.virtualWrite(V0, 0);          // LED merah di Blynk
    isNotified = false;                 // Reset status notifikasi
  } else {
    // Jika kondisi buruk, nyalakan LED merah dan alarm
    digitalWrite(LED_HIJAU, LOW);
    digitalWrite(LED_MERAH, HIGH);
    Blynk.virtualWrite(V4, 0);          // LED hijau di Blynk
    Blynk.virtualWrite(V0, 255);        // LED merah di Blynk

    // Nada alarm
    for (int i = 0; i < 5; i++) {
      tone(BUZZER_PIN, 1000);
      delay(200);
      noTone(BUZZER_PIN);
      delay(200);
      tone(BUZZER_PIN, 1500);
      delay(200);
      noTone(BUZZER_PIN);
      delay(200);
    }

    // Kirim notifikasi ke Telegram jika kondisi buruk dan belum ada notifikasi
    if (!isNotified) {
      String message = "WARNING!\nTemperature (C): " + String(t) + 
                       "\nHumidity (%): " + String(h) + 
                       "\nAir Quality: " + String(mq135Value);
      for (int i = 0; i < chatIdCount; i++) {
        bot.sendMessage(chatIds[i], message);
      }
      isNotified = true;  // Tandai bahwa notifikasi sudah dikirim
    }
  }
}

void loop() {
  Blynk.run();
  timer.run(); // Menjalankan timer untuk pembaruan data berkala
}
