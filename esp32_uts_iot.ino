/*
  ==============================================================
  TUGAS UTS IOT - SISTEM AGENT AI KENDALI 6 PERALATAN & SENSOR
  ==============================================================
  Hardware:
  - ESP32 Development Board
  - Sensor DHT22 / DHT11 (Suhu & Kelembaban) -> GPIO 4
  - Sensor Ultrasonik HC-SR04 (Jarak):
      * TRIG_PIN -> GPIO 5
      * ECHO_PIN -> GPIO 18
  - 6 LED (Peralatan Listrik):
      * LED 1 (Lampu)      -> GPIO 19
      * LED 2 (TV)         -> GPIO 21
      * LED 3 (Kulkas)     -> GPIO 22
      * LED 4 (Mesin Cuci) -> GPIO 23
      * LED 5 (AC)         -> GPIO 25
      * LED 6 (Radio)      -> GPIO 26
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
// Contoh jika Vercel: "https://iot-puce-nine.vercel.app/api"
// Contoh jika Local (satu Wi-Fi): "http://192.168.1.XX:3000/api"
const char* serverApiUrl = "https://iot-puce-nine.vercel.app/api";

// 3. Konfigurasi Sensor DHT22 / DHT11 (Suhu & Kelembaban)
#define DHTPIN 4
#define DHTTYPE DHT22 // Ganti ke DHT11 jika menggunakan modul DHT11 biru
DHT dht(DHTPIN, DHTTYPE);

// 4. Konfigurasi Sensor Ultrasonik HC-SR04
#define TRIG_PIN 5
#define ECHO_PIN 18

// 5. Konfigurasi 6 Pin LED (Peralatan Listrik)
#define PIN_LED_LAMPU       19  // LED 1: D19
#define PIN_LED_TV          21  // LED 2: D21
#define PIN_LED_KULKAS      22  // LED 3: D22
#define PIN_LED_MESIN_CUCI  23  // LED 4: D23
#define PIN_LED_AC          25  // LED 5: D25
#define PIN_LED_RADIO       26  // LED 6: D26

unsigned long lastTime = 0;
const unsigned long timerDelay = 1500; // Polling cepat setiap 1.5 detik agar respon LED instan

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

// Fungsi eksekusi langsung ke 6 Pin LED fisik
void setPhysicalLeds(bool stLampu, bool stTv, bool stKulkas, bool stMesinCuci, bool stAc, bool stRadio) {
  digitalWrite(PIN_LED_LAMPU, stLampu ? HIGH : LOW);
  digitalWrite(PIN_LED_TV, stTv ? HIGH : LOW);
  digitalWrite(PIN_LED_KULKAS, stKulkas ? HIGH : LOW);
  digitalWrite(PIN_LED_MESIN_CUCI, stMesinCuci ? HIGH : LOW);
  digitalWrite(PIN_LED_AC, stAc ? HIGH : LOW);
  digitalWrite(PIN_LED_RADIO, stRadio ? HIGH : LOW);
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
  setPhysicalLeds(false, false, false, false, false, false);

  // Hubungkan ke Wi-Fi
  Serial.print("Menghubungkan ke Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi Terhubung!");
    Serial.print("IP Address ESP32: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[INFO] Wi-Fi belum terhubung. Mode kendali Serial & Polling aktif.");
  }
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
    Serial.printf("[POST /api/sensor] Status: %d | T: %.1fC H: %.1f%% D: %.1fcm\n", httpResponseCode, suhu, kelembaban, jarak);
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

        setPhysicalLeds(stLampu, stTv, stKulkas, stMesinCuci, stAc, stRadio);

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

// 3. Listener Perintah Serial (USB COM Port) untuk pengujian langsung tanpa Wi-Fi
void handleSerialCommands() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();
    if (cmd.length() == 0) return;

    Serial.printf("[SERIAL CMD] Diterima: %s\n", cmd.c_str());

    if (cmd.indexOf("LAMPU:ON") >= 0 || cmd.indexOf("LED1:ON") >= 0 || cmd == "1:1") digitalWrite(PIN_LED_LAMPU, HIGH);
    else if (cmd.indexOf("LAMPU:OFF") >= 0 || cmd.indexOf("LED1:OFF") >= 0 || cmd == "1:0") digitalWrite(PIN_LED_LAMPU, LOW);

    if (cmd.indexOf("TV:ON") >= 0 || cmd.indexOf("LED2:ON") >= 0 || cmd == "2:1") digitalWrite(PIN_LED_TV, HIGH);
    else if (cmd.indexOf("TV:OFF") >= 0 || cmd.indexOf("LED2:OFF") >= 0 || cmd == "2:0") digitalWrite(PIN_LED_TV, LOW);

    if (cmd.indexOf("KULKAS:ON") >= 0 || cmd.indexOf("LED3:ON") >= 0 || cmd == "3:1") digitalWrite(PIN_LED_KULKAS, HIGH);
    else if (cmd.indexOf("KULKAS:OFF") >= 0 || cmd.indexOf("LED3:OFF") >= 0 || cmd == "3:0") digitalWrite(PIN_LED_KULKAS, LOW);

    if (cmd.indexOf("MESIN:ON") >= 0 || cmd.indexOf("LED4:ON") >= 0 || cmd == "4:1") digitalWrite(PIN_LED_MESIN_CUCI, HIGH);
    else if (cmd.indexOf("MESIN:OFF") >= 0 || cmd.indexOf("LED4:OFF") >= 0 || cmd == "4:0") digitalWrite(PIN_LED_MESIN_CUCI, LOW);

    if (cmd.indexOf("AC:ON") >= 0 || cmd.indexOf("LED5:ON") >= 0 || cmd == "5:1") digitalWrite(PIN_LED_AC, HIGH);
    else if (cmd.indexOf("AC:OFF") >= 0 || cmd.indexOf("LED5:OFF") >= 0 || cmd == "5:0") digitalWrite(PIN_LED_AC, LOW);

    if (cmd.indexOf("RADIO:ON") >= 0 || cmd.indexOf("LED6:ON") >= 0 || cmd == "6:1") digitalWrite(PIN_LED_RADIO, HIGH);
    else if (cmd.indexOf("RADIO:OFF") >= 0 || cmd.indexOf("LED6:OFF") >= 0 || cmd == "6:0") digitalWrite(PIN_LED_RADIO, LOW);

    if (cmd.indexOf("ALL:ON") >= 0 || cmd == "ALL1") setPhysicalLeds(true, true, true, true, true, true);
    else if (cmd.indexOf("ALL:OFF") >= 0 || cmd == "ALL0") setPhysicalLeds(false, false, false, false, false, false);
  }
}

void loop() {
  // Listener perintah cepat lewat USB Serial COM Port
  handleSerialCommands();

  if ((millis() - lastTime) > timerDelay) {
    lastTime = millis(); // Selalu perbarui timer agar tidak terkunci

    // 1. Coba baca Sensor Fisik
    float suhu = dht.readTemperature();
    float kelembaban = dht.readHumidity();
    float jarak = readUltrasonicDistance();

    if (!isnan(suhu) && !isnan(kelembaban)) {
      // Kirim data sensor jika sensor fisik sudah siap dan valid
      sendSensorData(suhu, kelembaban, jarak);
    } else {
      // Jangan blokir loop jika sensor belum terpasang atau sedang proses inisialisasi
      Serial.println("[INFO] Sensor DHT membaca nil / persiapan...");
    }

    // 2. Ambil perintah kendali 6 LED dari Server (Selalu dijalankan!)
    fetchAndControlLeds();
  }
}

