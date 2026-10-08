"""Restore the official pinned Win64 Ultimate ASI Loader (no PDB build)."""
import hashlib
import io
from pathlib import Path
import urllib.request
import zipfile

VERSION = '9.7.4'
URL = f'https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/v{VERSION}/Ultimate-ASI-Loader-NoPDB_x64.zip'
SHA256 = 'e5860e7d9a1805267535b65749575b5e406cc6ea3325c7392189c578815045d1'
ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / '.deps' / f'Ultimate-ASI-Loader-{VERSION}'

def restore():
    archive = DEST / 'loader.zip'
    payload = archive.read_bytes() if archive.exists() else urllib.request.urlopen(URL, timeout=60).read()
    if hashlib.sha256(payload).hexdigest() != SHA256:
        raise RuntimeError('Ultimate ASI Loader archive SHA256 mismatch')
    DEST.mkdir(parents=True, exist_ok=True)
    archive.write_bytes(payload)
    with zipfile.ZipFile(io.BytesIO(payload)) as bundle:
        # The generic proxy supports winmm.dll; distribute it under that name.
        binary = bundle.read('dinput8.dll')
    (DEST / 'winmm.dll').write_bytes(binary)
    print(f'Ultimate ASI Loader {VERSION} restored: {DEST / "winmm.dll"}')

if __name__ == '__main__':
    restore()
