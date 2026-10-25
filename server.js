// Runs on the computer that plays the music.
// Phones open https://<this computer's IP>:8443 and report which tag they see;
// the player page (https://localhost:8443/player.html) plays the matching song.
const https = require('https');
const fs = require('fs');
const os = require('os');
const path = require('path');
const selfsigned = require('selfsigned');

const PORT = 8443;
const ROOTS = { '/songs/': path.join(__dirname, 'songs'), '/': path.join(__dirname, 'public') };
const TYPES = { '.html': 'text/html', '.js': 'text/javascript', '.mp3': 'audio/mpeg', '.svg': 'image/svg+xml' };

// Phones only allow the camera on HTTPS pages, so serve with a self-signed certificate.
const certFile = path.join(__dirname, '.cert.json');
if (!fs.existsSync(certFile)) {
  fs.writeFileSync(certFile, JSON.stringify(selfsigned.generate([{ name: 'commonName', value: 'esc-snowglobe' }], { days: 3650, keySize: 2048 })));
}
const cert = JSON.parse(fs.readFileSync(certFile));

let currentTag = null;
const players = new Set();

function sendTag(res) {
  res.write(`data: ${JSON.stringify(currentTag)}\n\n`);
}

function serveFile(req, res) {
  const urlPath = decodeURIComponent(new URL(req.url, 'https://x').pathname);
  const prefix = Object.keys(ROOTS).find(p => urlPath.startsWith(p));
  const root = ROOTS[prefix];
  const file = path.join(root, urlPath.slice(prefix.length) || 'index.html');
  if (!file.startsWith(root)) return res.writeHead(403).end();
  fs.readFile(file, (err, data) => {
    if (err) return res.writeHead(404).end('Not found');
    res.writeHead(200, { 'Content-Type': TYPES[path.extname(file)] || 'application/octet-stream' }).end(data);
  });
}

https.createServer({ key: cert.private, cert: cert.cert }, (req, res) => {
  // Phone -> server: the tag it sees now (0-3), or null when none.
  if (req.method === 'POST' && req.url === '/tag') {
    let body = '';
    req.on('data', chunk => body += chunk);
    req.on('end', () => {
      currentTag = JSON.parse(body).tag;
      console.log(currentTag === null ? 'Stop' : `Tag ${currentTag}`);
      players.forEach(sendTag);
      res.writeHead(204).end();
    });
    return;
  }
  // Server -> player page: a live stream of tag changes.
  if (req.url === '/events') {
    res.writeHead(200, { 'Content-Type': 'text/event-stream', 'Cache-Control': 'no-cache' });
    players.add(res);
    sendTag(res);
    req.on('close', () => players.delete(res));
    return;
  }
  serveFile(req, res);
}).listen(PORT, () => {
  const ips = Object.values(os.networkInterfaces()).flat().filter(i => i.family === 'IPv4' && !i.internal).map(i => i.address);
  console.log(`Player (open on this computer): https://localhost:${PORT}/player.html`);
  ips.forEach(ip => console.log(`Phones (same WiFi):            https://${ip}:${PORT}`));
});
