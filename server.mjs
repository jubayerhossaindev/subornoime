import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const rootDir = path.dirname(fileURLToPath(import.meta.url));
const port = Number(process.env.PORT || 3000);
const host = '0.0.0.0';

const mimeTypes = new Map([
  ['.html', 'text/html; charset=utf-8'],
  ['.json', 'application/json; charset=utf-8'],
  ['.png', 'image/png'],
  ['.svg', 'image/svg+xml'],
  ['.ico', 'image/x-icon'],
  ['.txt', 'text/plain; charset=utf-8'],
]);

function readJsonSafe(relativePath) {
  try {
    const raw = fs.readFileSync(path.join(rootDir, relativePath), 'utf8');
    return JSON.parse(raw);
  } catch {
    return null;
  }
}

function escapeHtml(value) {
  return String(value)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function renderIndex() {
  const enStrings = readJsonSafe('data/i18n/en/ui.json') || {};
  const checkmap = readJsonSafe('scripts/windows-ui-audit/functional/checkmap.json');
  const checks = checkmap?.map ? Object.keys(checkmap.map) : [];

  const title = enStrings['settings.window.title'] || 'Suborno Settings';
  const surfaces = [
    { name: 'Settings Window', cls: 'SubornoDesktopWindow', tabs: 'General, Interface, Topbar, Input' },
    { name: 'Floating Topbar', cls: 'SubornoTopbarWindow', tabs: 'Mode, Layout, Wrench, Close' },
    { name: 'On-Screen Keyboard', cls: 'SubornoOskWindow', tabs: 'English / Bangla modes & keys' },
    { name: 'About Dialog', cls: 'SubornoAboutWindow', tabs: 'Updates, Docs, License, Notices' },
    { name: 'Dropdown Menu Popup', cls: 'SubornoMenuPopupWindow', tabs: 'Modeless UIA ListItems' },
    { name: 'Toast Notification', cls: 'SubornoToastWindow', tabs: 'Live save & locale feedback' },
  ];

  const surfaceRows = surfaces
    .map(
      (surface) => `
        <div class="card">
          <div class="card-title">${escapeHtml(surface.name)}</div>
          <div class="card-meta">${escapeHtml(surface.cls)}</div>
          <div class="card-desc">${escapeHtml(surface.tabs)}</div>
        </div>`,
    )
    .join('\n');

  const checkPreview = checks
    .slice(0, 24)
    .map((id) => `<li><code>${escapeHtml(id)}</code></li>`)
    .join('\n');

  return `<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>${escapeHtml(title)} — Suborno IME</title>
  <style>
    :root {
      color-scheme: dark;
      --bg: #0f1218;
      --surface: #171b24;
      --border: #272d3b;
      --text: #eef1f6;
      --muted: #98a2b3;
      --accent: #35d6c2;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: var(--bg);
      color: var(--text);
      line-height: 1.5;
    }
    .container {
      max-width: 960px;
      margin: 0 auto;
      padding: 40px 24px 64px;
    }
    header {
      display: flex;
      align-items: center;
      gap: 20px;
      margin-bottom: 24px;
    }
    header img.logo {
      width: 64px;
      height: 64px;
      border-radius: 14px;
    }
    h1 {
      margin: 0 0 4px;
      font-size: 28px;
      letter-spacing: -0.02em;
    }
    .subtitle {
      margin: 0;
      color: var(--muted);
    }
    .banner {
      width: 100%;
      border-radius: 12px;
      border: 1px solid var(--border);
      margin-bottom: 32px;
      display: block;
    }
    h2 {
      margin: 32px 0 14px;
      font-size: 18px;
      color: var(--accent);
    }
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
      gap: 14px;
    }
    .card {
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 16px;
    }
    .card-title {
      font-weight: 600;
      margin-bottom: 4px;
    }
    .card-meta {
      font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
      font-size: 12px;
      color: var(--accent);
      margin-bottom: 6px;
    }
    .card-desc {
      font-size: 13px;
      color: var(--muted);
    }
    .checks {
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 16px 20px;
    }
    .checks ul {
      list-style: none;
      margin: 12px 0 0;
      padding: 0;
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
      gap: 8px 16px;
    }
    code {
      font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
      font-size: 12px;
      color: var(--text);
    }
  </style>
</head>
<body>
  <main class="container">
    <header>
      <img class="logo" src="/assets/suborno-logo.png" alt="Suborno logo" />
      <div>
        <h1>Suborno IME</h1>
        <p class="subtitle">Native Bangla Input Method for Windows (C++20 / TSF)</p>
      </div>
    </header>
    <img class="banner" src="/assets/suborno-readme-banner.svg" alt="Suborno banner" />
    <section>
      <h2>Windows Desktop Surfaces</h2>
      <div class="grid">
        ${surfaceRows}
      </div>
    </section>
    <section>
      <h2>UI Automation Audit Coverage (${checks.length} mapped checks)</h2>
      <div class="checks">
        <div class="card-desc">
          Source: <code>scripts/windows-ui-audit/functional/checkmap.json</code> &amp;
          <code>scripts/windows-ui-audit/Invoke-SubornoUiAudit.ps1</code>
        </div>
        <ul>
          ${checkPreview}
        </ul>
      </div>
    </section>
  </main>
</body>
</html>`;
}

const server = http.createServer((request, response) => {
  const url = new URL(request.url || '/', `http://${request.headers.host || 'localhost'}`);

  if (url.pathname === '/healthz') {
    response.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8' });
    response.end(JSON.stringify({ ok: true }));
    return;
  }

  if (url.pathname === '/' || url.pathname === '/index.html') {
    const html = renderIndex();
    response.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
    response.end(html);
    return;
  }

  if (url.pathname.startsWith('/assets/')) {
    const safeRelative = path.normalize(url.pathname).replace(/^[/\\]+/, '');
    const fullPath = path.resolve(rootDir, safeRelative);
    const assetsRoot = path.resolve(rootDir, 'assets');
    if (fullPath.startsWith(assetsRoot + path.sep) && fs.existsSync(fullPath) && fs.statSync(fullPath).isFile()) {
      const ext = path.extname(fullPath).toLowerCase();
      response.writeHead(200, { 'Content-Type': mimeTypes.get(ext) || 'application/octet-stream' });
      fs.createReadStream(fullPath).pipe(response);
      return;
    }
  }

  response.writeHead(404, { 'Content-Type': 'application/json; charset=utf-8' });
  response.end(JSON.stringify({ error: 'Not found' }));
});

server.listen(port, host, () => {
  console.log(`Suborno preview server listening on http://${host}:${port}`);
});

function shutdown() {
  server.close(() => {
    process.exit(0);
  });
}

process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);
