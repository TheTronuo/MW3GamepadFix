"""Restore MinHook source from its upstream release, without installing anything."""
import hashlib
import io
from pathlib import Path
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DESTINATION = ROOT / ".deps" / "minhook-1.3.4"
URL = "https://codeload.github.com/TsudaKageyu/minhook/zip/refs/tags/v1.3.4"
EXPECTED_SHA256 = "172708123daa0c98d20d3a980b16a50be14af243dc95dee6f79c24193ad010e4"

def main():
    payload = urllib.request.urlopen(URL, timeout=30).read()
    digest = hashlib.sha256(payload).hexdigest()
    if digest != EXPECTED_SHA256:
        raise RuntimeError(f"Package hash mismatch: {digest}")
    with zipfile.ZipFile(io.BytesIO(payload)) as package:
        for member in package.infolist():
            if member.is_dir():
                continue
            relative = Path(*Path(member.filename).parts[1:])
            target = (DESTINATION / relative).resolve()
            if not target.is_relative_to(DESTINATION.resolve()):
                raise RuntimeError(f"Unsafe package entry: {member.filename}")
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(package.read(member))
    (DESTINATION / "package.sha256").write_text(digest + "\n", encoding="ascii")
    print(f"Restored MinHook 1.3.4; SHA256 {digest}")

if __name__ == "__main__":
    main()
