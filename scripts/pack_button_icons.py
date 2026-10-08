"""Pack original X360 or PS3 button pixels only; no localized letters."""
from pathlib import Path
import argparse
import csv, hashlib, json, struct
from PIL import Image

def main() -> None:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--atlas', type=Path, required=True, help='Source font atlas (PNG)')
    parser.add_argument('--glyphs', type=Path, required=True, help='Source glyph metrics (TSV)')
    parser.add_argument('--style', choices=('x360', 'ps3'), default='x360', help='Target prompt style')
    args = parser.parse_args()
    original = Image.open(args.atlas).convert('RGBA')
    with args.glyphs.open(encoding='utf8') as metrics:
        rows = {int(r['code']): r for r in csv.DictReader(metrics, delimiter='\t')}
    codes = list(range(1, 7)) + list(range(14, 24))
    atlas = Image.new('RGBA', (256, 256))
    records = []
    for index, code in enumerate(codes):
        row = rows[code]
        x, y = index % 4 * 64 + 2, index // 4 * 64 + 2
        w, h = int(row['width']), int(row['height'])
        sx, sy = int(float(row['s0']) * original.width), int(float(row['t0']) * original.height)
        if not (0 < w <= 60 and 0 < h <= 60):
            raise ValueError(f'Invalid button dimensions: {code}')
        atlas.paste(original.crop((sx, sy, sx + w, sy + h)), (x, y))
        records.append(struct.pack('<HbbBBBB4f', code, int(row['x0']), int(row['y0']),
            int(row['dx']), w, h, 0, (x + .5) / 256, (y + .5) / 256,
            (x + w + .5) / 256, (y + h + .5) / 256))
    pixels = atlas.tobytes('raw', 'BGRA')
    stem = 'button-icons-ps3' if args.style == 'ps3' else 'button-icons'
    target = root / 'assets' / f'{stem}.bin'
    target.write_bytes(struct.pack('<4s5I', b'BGP1', 256, 256, 33, len(records), len(pixels)) + b''.join(records) + pixels)
    (root / 'assets' / f'{stem}.json').write_text(json.dumps({'format': 'BGP1', 'width': 256,
        'height': 256, 'button_codes': codes, 'localized_letters': 0,
        'source': 'Original PS3 button pixels, not its localized font' if args.style == 'ps3'
                  else 'Original Xbox button pixels, not its localized font',
        'sha256': hashlib.sha256(target.read_bytes()).hexdigest()}, indent=2) + '\n', encoding='utf8')
    print('Packed', target, target.stat().st_size, 'bytes; 16 icons, no letters')


if __name__ == '__main__':
    main()
