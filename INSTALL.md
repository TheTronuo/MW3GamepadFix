# MW3GamepadFix 0.8.3

Gamepad support for Call of Duty: Modern Warfare 3 (2011), single-player on 64-bit Windows.
This is a pre-release; full campaign verification is ongoing.

## Install

Close the game and extract this package into the folder containing iw5sp.exe.
Place winmm.dll, MW3GamepadFix.asi and MW3GamepadFix.ini next to that executable.
Launch single-player normally. Game archives and the executable are not modified.
If another mod already provides winmm.dll, keep your existing compatible ASI loader.

The supported iw5sp.exe SHA256 is:
a97d2bbc7e495e4cf1a3b7e9a021e25b2c7b461d63f2b88d3cfe8782e8dc1023
Other executable builds and multiplayer are not supported.

## Settings

Edit MW3GamepadFix.ini, then restart the game.
- enabled=1 enables the mod; enabled=0 disables it.
- backend=xinput is the default. gameinput and auto are also available.
- prompts=x360 selects Xbox icons; prompts=ps3 selects PlayStation icons.
GameInput requires its Microsoft runtime. Prompt style is independent of the backend and game language.

## Uninstall

Close the game and remove MW3GamepadFix.asi, MW3GamepadFix.ini and the supplied winmm.dll.
Keep a shared ASI loader if other installed mods use it.

Source, updates and compatibility details: https://github.com/TheTronuo/MW3GamepadFix
Dependency licenses and original asset provenance: THIRD_PARTY_NOTICES.md and licenses/.
