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
