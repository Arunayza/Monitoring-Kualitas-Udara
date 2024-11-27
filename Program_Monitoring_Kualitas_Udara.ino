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
const char* ssid = "virus.exe";
const char* password = "virus136";

// Informasi Telegram
const char* telegramToken = "6470706945:AAGfRs87HNz3YnuiWyFbFPfoGmSIxjsZCh0";
const char* chatIds[] = {"1634461546", "6298395046"}; // Tambahkan ID chat di sini
const int chatIdCount = sizeof(chatIds) / sizeof(chatIds[0]);    // Menghitung jumlah chat ID

DHT dht(DHTPIN, DHTTYPE);               // Inisialisasi sensor DHT11
BlynkTimer timer;                       // Inisialisasi timer untuk Blynk
WiFiClientSecure client;                // Inisialisasi klien Wi-Fi dengan koneksi aman
UniversalTelegramBot bot(telegramToken, client); // Inisialisasi bot Telegram

const int AQ_THRESHOLD = 150;           // Ambang batas kualitas udara
bool isNotified = false;                // Status notifikasi

void setup() {
  Serial.begin(9600);                   // Inisialisasi komunikasi serial
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password); // Menghubungkan ke server Blynk
  dht.begin();                          // Inisialisasi sensor DHT11
  
  pinMode(LED_HIJAU, OUTPUT);           // Mengatur pin LED hijau sebagai output
  pinMode(LED_MERAH, OUTPUT);           // Mengatur pin LED merah sebagai output
  pinMode(BUZZER_PIN, OUTPUT);          // Mengatur pin buzzer sebagai output

  digitalWrite(LED_HIJAU, LOW);         // Mematikan LED hijau pada awal
  digitalWrite(LED_MERAH, LOW);         // Mematikan LED merah pada awal
  digitalWrite(BUZZER_PIN, LOW);        // Mematikan buzzer pada awal

  client.setInsecure();                 // Menghindari sertifikat untuk bot Telegram
  
  // Mengirim pesan "ONLINE" ke semua chat ID Telegram
  for (int i = 0; i < chatIdCount; i++) {
    bot.sendMessage(chatIds[i], "ESP8266 ONLINE");
  }

  // Menjadwalkan fungsi `sendData` untuk berjalan setiap 3 detik
  timer.setInterval(3000L, sendData);
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

  // Logika pengambilan keputusan
  bool gasTinggi = mq135Value > AQ_THRESHOLD;
  bool suhuTinggi = t > 30;
  bool kelembapanTidakNormal = (h < 40 || h > 65);

  // Kondisi gabungan
  if (!gasTinggi && !suhuTinggi && !kelembapanTidakNormal) {
    // Semua kondisi normal
    digitalWrite(LED_HIJAU, HIGH);
    digitalWrite(LED_MERAH, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Blynk.virtualWrite(V4, 255); // LED hijau di Blynk
    Blynk.virtualWrite(V0, 0);   // LED merah di Blynk
    isNotified = false;          // Reset status notifikasi
  } else if (gasTinggi && suhuTinggi && kelembapanTidakNormal) {
    // Semua kondisi buruk
    digitalWrite(LED_HIJAU, LOW);
    digitalWrite(LED_MERAH, HIGH);
    Blynk.virtualWrite(V4, 0);   // LED hijau di Blynk
    Blynk.virtualWrite(V0, 255); // LED merah di Blynk
    soundAlarm();                // Panggil fungsi alarm
    sendTelegramNotification(t, h, mq135Value, "CRITICAL: Semua kondisi buruk!");
  } else if (gasTinggi && suhuTinggi) {
    // Gas dan suhu tinggi
    digitalWrite(LED_HIJAU, LOW);
    digitalWrite(LED_MERAH, HIGH);
    Blynk.virtualWrite(V4, 0);
    Blynk.virtualWrite(V0, 255);
    soundAlarm();
    sendTelegramNotification(t, h, mq135Value, "WARNING: Gas dan suhu tinggi!");
  } else if (gasTinggi && kelembapanTidakNormal) {
    // Gas tinggi dan kelembapan tidak normal
    digitalWrite(LED_HIJAU, LOW);
    digitalWrite(LED_MERAH, HIGH);
    Blynk.virtualWrite(V4, 0);
    Blynk.virtualWrite(V0, 255);
    soundAlarm();
    sendTelegramNotification(t, h, mq135Value, "WARNING: Gas tinggi dan kelembapan tidak normal!");
  } else if (gasTinggi) {
    // Hanya gas tinggi
    digitalWrite(LED_HIJAU, LOW);
    digitalWrite(LED_MERAH, HIGH);
    Blynk.virtualWrite(V4, 0);
    Blynk.virtualWrite(V0, 255);
    soundAlarm();
    sendTelegramNotification(t, h, mq135Value, "WARNING: Gas tinggi!");
  } else if (suhuTinggi || kelembapanTidakNormal) {
    // Suhu tinggi atau kelembapan tidak normal (kondisi kurang nyaman)
    digitalWrite(LED_HIJAU, LOW);
    digitalWrite(LED_MERAH, LOW);
    Blynk.virtualWrite(V4, 128); // LED hijau setengah menyala (indikasi waspada)
    Blynk.virtualWrite(V0, 128); // LED merah setengah menyala (indikasi waspada)
    if (!isNotified) {
      sendTelegramNotification(t, h, mq135Value, "INFO: Suhu tinggi atau kelembapan tidak normal.");
      isNotified = true;
    }
  }
}

void soundAlarm() {
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
}

void sendTelegramNotification(float t, float h, int mq135Value, String condition) {
  if (!isNotified) {
    String message = condition + "\nTemperature (C): " + String(t) + 
                     "\nHumidity (%): " + String(h) + 
                     "\nAir Quality: " + String(mq135Value);
    for (int i = 0; i < chatIdCount; i++) {
      bot.sendMessage(chatIds[i], message);
    }
    isNotified = true;
  }
}

void loop() {
  Blynk.run();
  timer.run();  // Menjalankan timer untuk pembaruan data berkala
}
