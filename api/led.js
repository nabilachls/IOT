// State 6 peralatan elektronik (LED di ESP32)
let ledState = {
    lampu: "OFF",
    tv: "OFF",
    kulkas: "OFF",
    mesin_cuci: "OFF",
    ac: "OFF",
    radio: "OFF"
};

const CLOUD_STORE_URL = 'https://api.restful-api.dev/objects/ff808181a09d98f701a12d910d37427c';

// Sinkronisasi dengan cloud storage agar state tidak hilang saat Vercel cold-start
async function syncCloudStore(newState = null) {
    try {
        if (newState) {
            await fetch(CLOUD_STORE_URL, {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ name: 'uts_iot_led', data: newState })
            });
            Object.assign(ledState, newState);
        } else {
            const res = await fetch(CLOUD_STORE_URL);
            if (res.ok) {
                const json = await res.json();
                if (json && json.data) {
                    Object.assign(ledState, json.data);
                }
            }
        }
    } catch (e) {
        // Fallback in-memory
    }
}

export default async function handler(req, res) {
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

    // Ambil state tersimpan terbaru dari cloud store
    await syncCloudStore();

    let body = req.body || {};
    if (typeof body === 'string') {
        try { body = JSON.parse(body); } catch(e) { body = { text: body }; }
    }
    const query = req.query || {};

    // Ambil parameter alat dan status dari body atau query string
    let targetAlat = (body.alat || query.alat || '').toLowerCase();
    let targetStatus = (body.status || query.status || '').toUpperCase();

    // Natural Language fallback jika alat/status tidak terisi langsung
    const rawText = (body.message || body.command || body.text || '').toLowerCase();
    if (!targetAlat && rawText) {
        if (rawText.includes('lampu')) targetAlat = 'lampu';
        else if (rawText.includes('tv') || rawText.includes('televisi')) targetAlat = 'tv';
        else if (rawText.includes('kulkas')) targetAlat = 'kulkas';
        else if (rawText.includes('mesin cuci') || rawText.includes('cuci')) targetAlat = 'mesin_cuci';
        else if (rawText.includes('ac') || rawText.includes('pendingin')) targetAlat = 'ac';
        else if (rawText.includes('radio')) targetAlat = 'radio';
        else if (rawText.includes('semua')) body.all = rawText.includes('mati') ? 'OFF' : 'ON';

        if (!targetStatus) {
            targetStatus = (rawText.includes('mati') || rawText.includes('padam') || rawText.includes('off')) ? 'OFF' : 'ON';
        }
    }

    // Aksi 1: Ubah status spesifik
    if (targetAlat && (targetStatus === 'ON' || targetStatus === 'OFF' || targetStatus === '1' || targetStatus === '0')) {
        let key = targetAlat;
        if (key.includes('mesin') || key.includes('cuci')) key = 'mesin_cuci';
        else if (key.includes('lampu')) key = 'lampu';
        else if (key.includes('tv') || key.includes('televisi')) key = 'tv';
        else if (key.includes('kulkas')) key = 'kulkas';
        else if (key.includes('ac')) key = 'ac';
        else if (key.includes('radio')) key = 'radio';

        if (ledState[key] !== undefined) {
            ledState[key] = (targetStatus === 'ON' || targetStatus === '1') ? 'ON' : 'OFF';
            await syncCloudStore(ledState);
            return res.status(200).json({
                status: "success",
                pesan: `${key} berhasil diubah menjadi ${ledState[key]}`,
                alat: key,
                kondisi: ledState[key],
                peralatan: ledState
            });
        }
    }

    // Aksi 2: Semua alat sekaligus
    if (body.all !== undefined || query.all !== undefined) {
        const allSt = (((body.all || query.all).toUpperCase() === 'ON') || (body.all === '1')) ? 'ON' : 'OFF';
        Object.keys(ledState).forEach(k => ledState[k] = allSt);
        await syncCloudStore(ledState);
        return res.status(200).json({
            status: "success",
            pesan: `Seluruh 6 peralatan telah diubah menjadi ${allSt}`,
            peralatan: ledState
        });
    }

    // Aksi 3: Multi-keys object { lampu: 'ON', tv: 'OFF' }
    let changed = false;
    Object.keys(ledState).forEach(k => {
        if (body[k] !== undefined) {
            ledState[k] = (body[k].toString().toUpperCase() === 'ON' || body[k] === '1') ? 'ON' : 'OFF';
            changed = true;
        }
    });

    if (changed) {
        await syncCloudStore(ledState);
        return res.status(200).json({
            status: "success",
            pesan: "Status peralatan berhasil diperbarui",
            peralatan: ledState
        });
    }

    // Default GET / response status peralatan terkini (untuk ESP32 polling)
    if (req.method === 'GET') {
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

    return res.status(200).json({
        status: "success",
        peralatan: ledState
    });
}

