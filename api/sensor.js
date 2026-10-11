// Penyimpanan data sensor realtime murni dari ESP32 (Tanpa data dummy)
let sensorData = {
    suhu: null,
    kelembaban: null,
    jarak: null,
    satuan_suhu: "°C",
    satuan_kelembaban: "%",
    satuan_jarak: "cm",
    timestamp: null
};

export default function handler(req, res) {
    // CORS headers agar bisa dipanggil dari ESP32, n8n, dan browser
    res.setHeader('Access-Control-Allow-Credentials', true);
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET,OPTIONS,PATCH,DELETE,POST,PUT');
    res.setHeader(
        'Access-Control-Allow-Headers',
        'X-CSRF-Token, X-Requested-With, Accept, Accept-Version, Content-Length, Content-MD5, Content-Type, Date, X-Api-Version'
    );

    if (req.method === 'OPTIONS') {
        res.status(200).end();
        return;
    }

    // GET: Membaca nilai sensor terkini (dipanggil oleh Tool n8n AI Agent atau Web)
    if (req.method === 'GET') {
        if (sensorData.suhu === null) {
            return res.status(200).json({
                status: "waiting",
                timestamp: null,
                sensor: {
                    suhu: null,
                    kelembaban: null,
                    jarak: null,
                    satuan_suhu: "°C",
                    satuan_kelembaban: "%",
                    satuan_jarak: "cm",
                    ringkasan: "Menunggu pengiriman data sensor perdana dari ESP32 (DHT22 & HC-SR04)."
                }
            });
        }

        return res.status(200).json({
            status: "success",
            timestamp: sensorData.timestamp,
            sensor: {
                suhu: sensorData.suhu,
                kelembaban: sensorData.kelembaban,
                jarak: sensorData.jarak,
                satuan_suhu: sensorData.satuan_suhu,
                satuan_kelembaban: sensorData.satuan_kelembaban,
                satuan_jarak: sensorData.satuan_jarak,
                ringkasan: `Suhu ruangan ${sensorData.suhu} derajat Celcius, kelembaban udara ${sensorData.kelembaban} persen, dan jarak halangan ${sensorData.jarak} sentimeter.`
            }
        });
    }

    // POST: Memperbarui nilai sensor murni (dikirim oleh ESP32 DHT22 + HC-SR04)
    if (req.method === 'POST') {
        const body = req.body || {};
        if (body.suhu !== undefined && body.suhu !== null) sensorData.suhu = Number(body.suhu);
        if (body.kelembaban !== undefined && body.kelembaban !== null) sensorData.kelembaban = Number(body.kelembaban);
        if (body.jarak !== undefined && body.jarak !== null) sensorData.jarak = Number(body.jarak);
        sensorData.timestamp = new Date().toISOString();

        return res.status(200).json({
            status: "success",
            message: "Data sensor asli berhasil diterima dari ESP32",
            data: sensorData
        });
    }

    return res.status(405).json({ error: "Method not allowed" });
}
