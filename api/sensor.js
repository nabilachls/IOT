// Global in-memory storage untuk data sensor terkini
let sensorData = {
    suhu: 28.5,
    kelembaban: 65.2,
    jarak: 18.4,
    satuan_suhu: "°C",
    satuan_kelembaban: "%",
    satuan_jarak: "cm",
    timestamp: new Date().toISOString()
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
                ringkasan: `Suhu saat ini ${sensorData.suhu} derajat Celcius, kelembaban ${sensorData.kelembaban} persen, dan jarak objek sejauh ${sensorData.jarak} centimeter.`
            }
        });
    }

    // POST: Memperbarui nilai sensor (dipanggil oleh ESP32 DHT11 + HC-SR04)
    if (req.method === 'POST') {
        const body = req.body || {};
        if (body.suhu !== undefined) sensorData.suhu = Number(body.suhu);
        if (body.kelembaban !== undefined) sensorData.kelembaban = Number(body.kelembaban);
        if (body.jarak !== undefined) sensorData.jarak = Number(body.jarak);
        sensorData.timestamp = new Date().toISOString();

        return res.status(200).json({
            status: "success",
            message: "Data sensor berhasil diperbarui dari ESP32",
            data: sensorData
        });
    }

    return res.status(405).json({ error: "Method not allowed" });
}
