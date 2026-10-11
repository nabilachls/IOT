export default async function handler(req, res) {
    // Set CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    if (req.method === 'OPTIONS') {
        return res.status(200).end();
    }

    const { text } = req.query;
    const cleanText = (text || 'Halo').slice(0, 250);

    const googleTtsUrl = `https://translate.google.com/translate_tts?ie=UTF-8&tl=id&client=tw-ob&q=${encodeURIComponent(cleanText)}`;

    try {
        const response = await fetch(googleTtsUrl, {
            headers: {
                'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'
            }
        });

        if (!response.ok) {
            return res.status(502).json({ error: 'TTS upstream error' });
        }

        const arrayBuffer = await response.arrayBuffer();
        res.setHeader('Content-Type', 'audio/mpeg');
        res.setHeader('Cache-Control', 'public, max-age=86400');
        return res.status(200).send(Buffer.from(arrayBuffer));
    } catch (error) {
        return res.status(500).json({ error: error.message });
    }
}
