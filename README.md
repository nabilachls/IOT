# ⚡ UTS IoT - Lamban AI Voice Assistant & IoT Controller

Sistem asisten AI berbasis suara yang terintegrasi dengan alur kerja **n8n**, model **Google Gemini Flash Lite**, **Vercel Serverless API**, dan mikrokontroler **ESP32** untuk pemantauan telemetri sensor dan kendali 6 peralatan listrik berbasis LED.

---

## 🏛️ Arsitektur Sistem

```text
[ Pengguna ] 🎙️ (Suara STT)
     │
     ▼
[ Web Interface (index.html) ]
     │  (HTTP POST /jarvis-chat)
     ▼
[ n8n AI Agent (Gemini Flash Lite) ]
     ├── Tool 1: GET /api/sensor  ──> [ Vercel API / Sensor ] <── [ ESP32: DHT11 + HC-SR04 ]
     └── Tool 2: POST /api/led   ──> [ Vercel API / LED ]    ──> [ ESP32: 6 LED Peralatan ]
     │
     ▼
[ Respond to Webhook (JSON) ]
     │
     ▼
[ Web Interface ] 🔊 (Text to Speech Suara Bahasa Indonesia)
```

---

## 🔌 Pinout Hardware (ESP32)

| Komponen | Pin Hardware | Pin ESP32 | Keterangan & Daya |
| :--- | :--- | :--- | :--- |
| **DHT22** | DATA | `D4 (GPIO 4)` | VCC ke `3V3`, GND ke Breadboard |
| **Ultrasonik (HC-SR04)** | TRIG | `D5 (GPIO 5)` | VCC ke `VIN`, GND ke Breadboard |
| **Ultrasonik (HC-SR04)** | ECHO | `D18 (GPIO 18)` | Echo Pulse |
| **LED 1 (Lampu)** | ANODA (+) | `D19 (GPIO 19)` | Kontrol Lampu |
| **LED 2 (TV)** | ANODA (+) | `D21 (GPIO 21)` | Kontrol Televisi |
| **LED 3 (Kulkas)** | ANODA (+) | `D22 (GPIO 22)` | Kontrol Kulkas |
| **LED 4 (Mesin Cuci)** | ANODA (+) | `D23 (GPIO 23)` | Kontrol Mesin Cuci |
| **LED 5 (AC)** | ANODA (+) | `D25 (GPIO 25)` | Kontrol Pendingin Ruangan |
| **LED 6 (Radio)** | ANODA (+) | `D26 (GPIO 26)` | Kontrol Radio Audio |

---

## 🌐 Endpoint API Vercel

### 1. API Sensor (`/api/sensor`)
- **GET `/api/sensor`**: Membaca nilai suhu (°C), kelembaban (%), dan jarak (cm) terkini.
- **POST `/api/sensor`**: Menerima pengiriman data telemetri dari ESP32.
  ```json
  {
    "suhu": 28.5,
    "kelembaban": 65.2,
    "jarak": 18.4
  }
  ```

### 2. API Kendali LED (`/api/led`)
- **GET `/api/led`**: Membaca status ON/OFF seluruh peralatan (dipanggil ESP32).
- **POST `/api/led`**: Mengubah status peralatan (dipanggil oleh n8n AI Agent).
  ```json
  {
    "alat": "lampu",
    "status": "ON"
  }
  ```

---

## 🚀 Panduan Menjalankan

### 1. Deploy ke Vercel
```bash
npx vercel
```
Ikuti instruksi terminal hingga mendapatkan URL publik (misal: `https://iot-uts-api.vercel.app`).

### 2. Import Workflow n8n
1. Buka n8n di `http://localhost:5678`.
2. Klik menu **titik tiga (...)** ➔ **Import from File**.
3. Pilih file `n8n_workflow_uts_iot.json`.
4. Masukkan Google Gemini API Key Anda.
5. Klik **Publish**.

### 3. Program ESP32
1. Buka file `esp32_uts_iot.ino` di Arduino IDE.
2. Masukkan SSID dan Password Wi-Fi Anda.
3. Masukkan URL Vercel pada variabel `serverApiUrl`.
4. Upload program ke ESP32.
