# Dependencies

* MinHook 1.3.4 - Tsuda Kageyu, BSD-style license, including Hacker Disassembler Engine notices. Complete original text: `licenses/MinHook.txt`. Source: https://github.com/TsudaKageyu/minhook/releases/tag/v1.3.4
* Microsoft.GameInput SDK 3.5.283 - https://www.nuget.org/packages/Microsoft.GameInput/3.5.283 . The public header and loader source carry the MIT notice preserved in `licenses/GameInput-SDK-MIT.txt`. The package's original Microsoft redistributable terms are retained in `licenses/GameInput-Redistributable.txt`.
* XInput 1.4 - Windows / Windows SDK component. The project links the SDK import library and uses the operating system's DLL.
* Ultimate ASI Loader 9.7.4 - ThirteenAG, MIT license: `licenses/Ultimate-ASI-Loader.txt`. Official release: https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/tag/v9.7.4 . The Win64 NoPDB generic proxy is distributed as `winmm.dll`; the original filename in the archive is `dinput8.dll`. Archive SHA256: `e5860e7d9a1805267535b65749575b5e406cc6ea3325c7392189c578815045d1`; binary SHA256: `031a3e5576d91dce1e438d36b9a3d462c7334ab4791990a8ff1e3ddc0e132daf`.

GameInput's runtime installer and Windows system DLLs are not included in the release package. SDK downloads are pinned by SHA256 in the restore scripts. Original dependency files and their notices remain in `.deps`.

# Original game assets

The embedded controller atlases and glyph metrics were extracted from local Xbox 360 and PlayStation 3 MW3 copies (`code_post_gfx.ff`, `gamefonts_xenon` / `gamefonts_ps3`, `normalFont`). Asset provenance and hashes are recorded in `assets/button-icons.json` and `assets/button-icons-ps3.json`. These are original Call of Duty game assets, separate from the open-source dependency licenses above.

For orientation during reverse engineering, IW5 asset declarations were consulted in [OpenAssetTools](https://github.com/Laupetin/OpenAssetTools/blob/main/src/Common/Game/IW5/IW5_Assets.h). This project implements a small build-specific ABI with offsets verified against the inspected executable and live font/material/image objects; it does not bundle that header.
