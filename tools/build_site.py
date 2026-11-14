#!/usr/bin/env python3
"""Export only public player assets; never deploy the repository root."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'dist'

def build():
    html = (ROOT / 'wireless.html').read_text()
    songs = re.findall(r"file:'(songs/[^']+)'", html)
    # Four globe songs plus O Christmas Tree, which plays from a button.
    if len(songs) != 5 or len(set(songs)) != 5:
        raise ValueError('Expected exactly five distinct playlist files')
    assets = ['wireless.html', 'instructions.html', 'wireless.css', 'player.js', 'ble.js', 'lib/mqtt.min.js', 'lib/LICENSE-mqtt.md', *songs]
    for name in assets:
        source = (ROOT / name).resolve()
        if not source.is_relative_to(ROOT) or not source.is_file():
            raise ValueError('Missing or unsafe asset: ' + name)
        if source.stat().st_size >= 25 * 1024 * 1024:
            raise ValueError('Asset exceeds Pages file limit: ' + name)
    if OUT.is_symlink():
        raise ValueError('Refusing a symlinked build directory')
    if OUT.exists():
        shutil.rmtree(OUT)
    OUT.mkdir()
    digest = hashlib.sha256()
    for name in assets:
        dest = OUT / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / name, dest)
        digest.update(name.encode() + b'\0' + dest.read_bytes())
    (OUT / 'index.html').write_text(html)
    (OUT / '_headers').write_text("""/*
  X-Content-Type-Options: nosniff
  X-Frame-Options: DENY
  Referrer-Policy: strict-origin-when-cross-origin
  Permissions-Policy: bluetooth=(self)
""")
    commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    info = {'commit': commit, 'source_sha256': digest.hexdigest()}
    (OUT / 'build-info.json').write_text(json.dumps(info, indent=2) + '\n')
    print(json.dumps({'directory': str(OUT), **info}, indent=2))

if __name__ == '__main__':
    build()
