"""Build a copy-to-game-root ZIP, with no external injector or console program."""
import argparse
import hashlib
import json
import re
from pathlib import Path
import shutil
import struct
import zipfile
from restore_asi_loader import restore, DEST, URL, SHA256, VERSION

ROOT = Path(__file__).resolve().parents[1]
def copy_x64(source: Path, package: Path, name: str) -> None:
    data = source.read_bytes()
    pe = struct.unpack_from('<I', data, 0x3C)[0]
    if data[pe:pe+4] != b'PE\0\0' or struct.unpack_from('<H', data, pe+4)[0] != 0x8664:
        raise RuntimeError(f'Expected AMD64 PE: {source}')
    (package / name).write_bytes(data)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--build', type=Path, default=ROOT / 'out/build/vs2022-x64/Release')
    args = parser.parse_args()
    project_version = re.search(r'project\(MW3GamepadFix VERSION ([\d.]+)', (ROOT / 'CMakeLists.txt').read_text()).group(1)
    restore()
    package = ROOT / f'out/dist-asi-v{project_version}'
    package.mkdir(parents=True, exist_ok=True)

    copy_x64(DEST / 'winmm.dll', package, 'winmm.dll')
    copy_x64(args.build.resolve() / 'MW3GamepadFix.asi', package, 'MW3GamepadFix.asi')
    for name in ('MW3GamepadFix.ini', 'THIRD_PARTY_NOTICES.md'):
        shutil.copyfile(ROOT / name, package / name)
    shutil.copyfile(ROOT / 'INSTALL.md', package / 'README.md')
    (package / 'assets').mkdir(exist_ok=True)
    for name in ('button-icons.json', 'button-icons-ps3.json'):
        shutil.copyfile(ROOT / 'assets' / name, package / 'assets' / name)
    (package / 'licenses').mkdir(exist_ok=True)
    for source in (ROOT / 'licenses').iterdir():
        if source.is_file():
            shutil.copyfile(source, package / 'licenses' / source.name)
    manifest = {
        'version': project_version, 'supported_exe': 'iw5sp.exe',
        'supported_sha256': 'a97d2bbc7e495e4cf1a3b7e9a021e25b2c7b461d63f2b88d3cfe8782e8dc1023',
        'asi_loader': {'version': VERSION, 'url': URL, 'archive_sha256': SHA256},
        'files': {str(p.relative_to(package)).replace('\\', '/'): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(package.rglob('*')) if p.is_file() and p.name != 'manifest.json'},
    }
    (package / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf8')
    archive = ROOT / f'out/MW3GamepadFix-{project_version}-ASI.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as bundle:
        for name in sorted(manifest['files']):
            bundle.write(package / name, name)
        bundle.write(package / 'manifest.json', 'manifest.json')
    print(f'Package: {archive}')


if __name__ == '__main__':
    main()
