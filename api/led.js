// State 6 peralatan elektronik (LED di ESP32)
let ledState = {
    lampu: "OFF",
    tv: "OFF",
    kulkas: "OFF",
    mesin_cuci: "OFF",
    ac: "OFF",
    radio: "OFF"
};

export default function handler(req, res) {
    // Header CORS
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

    // GET: Membaca status 6 LED (dipanggil oleh ESP32 untuk menyalakan/mematikan pin GPIO LED)
    if (req.method === 'GET') {
        // Mendukung filter spesifik misal ?alat=lampu
        const { alat } = req.query;
        if (alat && ledState[alat.toLowerCase()] !== undefined) {
            return res.status(200).json({
                alat: alat.toLowerCase(),
                status: ledState[alat.toLowerCase()]
            });
        }

        return res.status(200).json({
            status: "success",
            peralatan: ledState,
            ringkasan: `Status peralatan: Lampu ${ledState.lampu}, TV ${ledState.tv}, Kulkas ${ledState.kulkas}, Mesin Cuci ${ledState.mesin_cuci}, AC ${ledState.ac}, Radio ${ledState.radio}.`
        });
    }

    // POST: Mengubah status LED (dipanggil oleh n8n AI Agent ketika ada perintah suara)
    if (req.method === 'POST') {
        const body = req.body || {};
        const query = req.query || {};

        // Format 1: { "alat": "lampu", "status": "ON" } atau via query ?alat=lampu&status=ON
        const targetAlat = (body.alat || query.alat || '').toLowerCase();
        const targetStatus = (body.status || query.status || '').toUpperCase();

        if (targetAlat) {
            // Normalisasi nama alat
            let key = targetAlat;
            if (key.includes('mesin') || key.includes('cuci')) key = 'mesin_cuci';
            else if (key.includes('lampu')) key = 'lampu';
            else if (key.includes('tv') || key.includes('televisi')) key = 'tv';
            else if (key.includes('kulkas')) key = 'kulkas';
            else if (key.includes('ac')) key = 'ac';
            else if (key.includes('radio')) key = 'radio';

            if (ledState[key] !== undefined) {
                ledState[key] = (targetStatus === 'ON' || targetStatus === '1' || targetStatus === 'TRUE') ? 'ON' : 'OFF';
                return res.status(200).json({
                    status: "success",
                    pesan: `${key} berhasil diubah menjadi ${ledState[key]}`,
                    alat: key,
                    kondisi: ledState[key],
                    peralatan: ledState
                });
            }
        }

        // Format 2: Perintah semua alat sekaligus { "all": "ON" }
        if (body.all !== undefined || query.all !== undefined) {
            const allSt = ((body.all || query.all).toUpperCase() === 'ON') ? 'ON' : 'OFF';
            Object.keys(ledState).forEach(k => ledState[k] = allSt);
            return res.status(200).json({
                status: "success",
                pesan: `Seluruh 6 peralatan telah diubah menjadi ${allSt}`,
                peralatan: ledState
            });
        }

        // Format 3: Kirim objek langsung { "lampu": "ON", "tv": "OFF" }
        let changed = false;
        Object.keys(ledState).forEach(k => {
            if (body[k] !== undefined) {
                ledState[k] = body[k].toUpperCase() === 'ON' ? 'ON' : 'OFF';
                changed = true;
            }
        });

        if (changed) {
            return res.status(200).json({
                status: "success",
                pesan: "Status peralatan berhasil diperbarui",
                peralatan: ledState
            });
        }

        return res.status(400).json({
            status: "error",
            message: "Parameter tidak dikenali. Gunakan { alat: 'lampu', status: 'ON' } atau { all: 'ON' }."
        });
    }

    return res.status(405).json({ error: "Method not allowed" });
}
