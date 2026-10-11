/*
  ==============================================================
  TUGAS UTS IOT - SISTEM AGENT AI KENDALI 6 PERALATAN & SENSOR
  ==============================================================
  Hardware:
  - ESP32 Development Board
  - Sensor DHT11 (Suhu & Kelembaban) -> GPIO 4
  - Sensor Ultrasonik HC-SR04 (Jarak):
      * TRIG_PIN -> GPIO 5
      * ECHO_PIN -> GPIO 18
  - 6 LED (Peralatan Listrik):
      * LED 1 (Lampu)      -> GPIO 16
      * LED 2 (TV)         -> GPIO 17
      * LED 3 (Kulkas)     -> GPIO 23
      * LED 4 (Mesin Cuci) -> GPIO 19
      * LED 5 (AC)         -> GPIO 21
      * LED 6 (Radio)      -> GPIO 22
  ==============================================================
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// 1. Kredensial Wi-Fi
const char* ssid = "NAMA_WIFI_ANDA";
const char* password = "PASSWORD_WIFI_ANDA";

// 2. URL API Vercel / Server Lokal
// Contoh jika Vercel: "https://iot-uts-api.vercel.app/api"
// Contoh jika Local (satu Wi-Fi): "http://192.168.1.XX:3000/api"
const char* serverApiUrl = "https://NAMA_PROJECT_ANDA.vercel.app/api";

// 3. Konfigurasi Sensor DHT11
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// 4. Konfigurasi Sensor Ultrasonik HC-SR04
#define TRIG_PIN 5
#define ECHO_PIN 18

// 5. Konfigurasi 6 Pin LED (Peralatan)
#define PIN_LED_LAMPU       16
#define PIN_LED_TV          17
#define PIN_LED_KULKAS      23
#define PIN_LED_MESIN_CUCI  19
#define PIN_LED_AC          21
#define PIN_LED_RADIO       22

unsigned long lastTime = 0;
const unsigned long timerDelay = 3000; // Kirim dan baca data setiap 3 detik

// Fungsi membaca jarak dari ultrasonik (cm)
float readUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms
  if (duration == 0) return 999.0; // Tidak ada halangan
  float distanceCm = duration * 0.034 / 2;
  return distanceCm;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Inisialisasi Sensor
  dht.begin();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Inisialisasi 6 Pin LED
  pinMode(PIN_LED_LAMPU, OUTPUT);
  pinMode(PIN_LED_TV, OUTPUT);
  pinMode(PIN_LED_KULKAS, OUTPUT);
  pinMode(PIN_LED_MESIN_CUCI, OUTPUT);
  pinMode(PIN_LED_AC, OUTPUT);
  pinMode(PIN_LED_RADIO, OUTPUT);

  // Matikan semua LED di awal
  digitalWrite(PIN_LED_LAMPU, LOW);
  digitalWrite(PIN_LED_TV, LOW);
  digitalWrite(PIN_LED_KULKAS, LOW);
  digitalWrite(PIN_LED_MESIN_CUCI, LOW);
  digitalWrite(PIN_LED_AC, LOW);
  digitalWrite(PIN_LED_RADIO, LOW);

  // Hubungkan ke Wi-Fi
  Serial.print("Menghubungkan ke Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi Terhubung!");
  Serial.print("IP Address ESP32: ");
  Serial.println(WiFi.localIP());
}

// 1. Fungsi Mengirim Data Sensor ke API (POST /api/sensor)
void sendSensorData(float suhu, float kelembaban, float jarak) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String endpoint = String(serverApiUrl) + "/sensor";
    http.begin(endpoint);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<200> doc;
    doc["suhu"] = suhu;
    doc["kelembaban"] = kelembaban;
    doc["jarak"] = jarak;

    String requestBody;
    serializeJson(doc, requestBody);

    int httpResponseCode = http.POST(requestBody);
    Serial.print("POST /api/sensor Status: ");
    Serial.println(httpResponseCode);
    http.end();
  }
}

// 2. Fungsi Mengambil Status 6 LED dari API (GET /api/led) & Kontrol Hardware
void fetchAndControlLeds() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String endpoint = String(serverApiUrl) + "/led";
    http.begin(endpoint);

    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      String response = http.getString();
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, response);

      if (!error) {
        JsonObject peralatan = doc["peralatan"];
        
        // Baca status masing-masing alat dan aktifkan pin LED
        bool stLampu     = String(peralatan["lampu"].as<const char*>()) == "ON";
        bool stTv        = String(peralatan["tv"].as<const char*>()) == "ON";
        bool stKulkas    = String(peralatan["kulkas"].as<const char*>()) == "ON";
        bool stMesinCuci = String(peralatan["mesin_cuci"].as<const char*>()) == "ON";
        bool stAc        = String(peralatan["ac"].as<const char*>()) == "ON";
        bool stRadio     = String(peralatan["radio"].as<const char*>()) == "ON";

        digitalWrite(PIN_LED_LAMPU, stLampu ? HIGH : LOW);
        digitalWrite(PIN_LED_TV, stTv ? HIGH : LOW);
        digitalWrite(PIN_LED_KULKAS, stKulkas ? HIGH : LOW);
        digitalWrite(PIN_LED_MESIN_CUCI, stMesinCuci ? HIGH : LOW);
        digitalWrite(PIN_LED_AC, stAc ? HIGH : LOW);
        digitalWrite(PIN_LED_RADIO, stRadio ? HIGH : LOW);

        Serial.printf("[STATUS LED] Lampu:%d TV:%d Kulkas:%d MesinCuci:%d AC:%d Radio:%d\n",
          stLampu, stTv, stKulkas, stMesinCuci, stAc, stRadio);
      }
    } else {
      Serial.print("GET /api/led Error: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // 1. Baca Sensor Fisik
    float suhu = dht.readTemperature();
    float kelembaban = dht.readHumidity();
    float jarak = readUltrasonicDistance();

    if (isnan(suhu) || isnan(kelembaban)) {
      Serial.println("Gagal membaca DHT11!");
      suhu = 28.5; // fallback
      kelembaban = 65.0;
    }

    Serial.printf("[SENSOR] Suhu: %.1f C | Kelembaban: %.1f %% | Jarak: %.1f cm\n", suhu, kelembaban, jarak);

    // 2. Kirim data sensor ke Server
    sendSensorData(suhu, kelembaban, jarak);

    // 3. Ambil perintah kendali 6 LED dari Server
    fetchAndControlLeds();

    lastTime = millis();
  }
}
