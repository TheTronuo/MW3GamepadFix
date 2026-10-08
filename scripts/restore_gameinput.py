"""Restore a pinned Microsoft SDK; never execute its runtime installer."""
import hashlib
import io
from pathlib import Path
import urllib.request
import zipfile

VERSION = "3.5.283"
EXPECTED_SHA256 = "b5988cb8ff9d7009b6ddf6ad4e3ff87e91b00cc17ffcd5d628dabc21c208f100"
ROOT = Path(__file__).resolve().parents[1]
DESTINATION = ROOT / ".deps" / f"Microsoft.GameInput.{VERSION}"
URL = f"https://api.nuget.org/v3-flatcontainer/microsoft.gameinput/{VERSION}/microsoft.gameinput.{VERSION}.nupkg"

def main():
    payload = urllib.request.urlopen(URL, timeout=30).read()
    digest = hashlib.sha256(payload).hexdigest()
    if digest != EXPECTED_SHA256:
        raise RuntimeError(f"Package hash mismatch: {digest}")
    with zipfile.ZipFile(io.BytesIO(payload)) as package:
        # SDK files and the unopened redistributable; no code is installed.
        for member in package.infolist():
            if member.is_dir():
                continue
            target = (DESTINATION / member.filename).resolve()
            if not target.is_relative_to(DESTINATION.resolve()):
                raise RuntimeError(f"Unsafe package entry: {member.filename}")
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(package.read(member))
    (DESTINATION / "package.sha256").write_text(digest + "\n", encoding="ascii")
    print(f"Restored Microsoft.GameInput {VERSION}; SHA256 {digest}")
    print(f"SDK: {DESTINATION}")

if __name__ == "__main__":
    main()
