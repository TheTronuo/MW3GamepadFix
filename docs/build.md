# Build

Requirements: Visual Studio 2022 with C++ tools and Windows SDK, CMake 3.25+, Python 3.10+, and 64-bit Windows.

Run from the repository root:

```powershell
python scripts/restore_minhook.py
python scripts/restore_gameinput.py
cmake --preset vs2022-x64
cmake --build --preset release
ctest --preset release
python scripts/package_asi.py
```

The restore scripts download pinned dependencies and verify their SHA256 hashes. The package script restores Ultimate ASI Loader and creates `out/MW3GamepadFix-<version>-ASI.zip`. Build output and dependencies are ignored by Git.

`assets/button-icons.bin` and `assets/button-icons-ps3.bin` are the embedded button resources. Repacking them is optional and requires Pillow plus separately supplied source images and metrics:

```powershell
python scripts/pack_button_icons.py --style <x360-or-ps3> --atlas <atlas.png> --glyphs <metrics.tsv>
```

See [third-party notices](../THIRD_PARTY_NOTICES.md) and `licenses/` for dependency licenses and asset provenance.
