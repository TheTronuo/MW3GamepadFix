# Compatibility and verification

The mod targets the Windows x64 single-player executable `iw5sp.exe` from Call of Duty: Modern Warfare 3 (2011).

Supported executable SHA256:

```text
a97d2bbc7e495e4cf1a3b7e9a021e25b2c7b461d63f2b88d3cfe8782e8dc1023
```

The executable hash and hook signatures are checked before hooks are enabled. Other executable builds and multiplayer are unsupported. The mod does not patch game files on disk.

## Version 0.8.4

The Release build passes all 16 CTest tests.

The SDV steering hint now reaches the existing key-based prompt conversion through the HUD formatter's direct localization lookup. The additional hook checks the verified lookup signature and is restricted to the formatter call site, localization asset type and `NY_HARBOR_PLATFORM_HINT_DRIVE_SDV_3` key. It returns a private, stable localization entry; database assets and translation bytes are not modified.

The HUD entry test covers Russian, English, Arabic, an already-expanded movement icon, keyboard/controller gating, unrelated lookups, repeated conversion and pointer lifetime across locale changes. The native asset lookup prologue is also covered by the MinHook relocation test. The right-stick icon in the SDV hint was confirmed working in gameplay by a user on October 8, 2026.

## Version 0.8.3

The Release build passes all 15 CTest tests, covering input handling, menus, gameplay actions, controller prompts, camera hooks, hook lifetime, ASI startup and remote steering.

Prompt coverage was reviewed against 93 installed single-player archives. The current catalog handles 63 prompt contexts and all 30 explicit action tokens identified by that review. Tests exercise English, Russian and Arabic text preservation, both button styles, repeated substitutions, and inline material records.

This archive review and the automated tests do not establish that every campaign scene works in gameplay. Full campaign, vehicle, mission-specific and Special Ops verification is ongoing; this release is marked as a pre-release.

Version 0.8.3 corrects Predator missile steering: pitch and yaw use the correct command fields and directions, with input from both sticks and vertical inversion support. Automated checks cover axis directions, stick mixing, rounding, saturation and input gating. Predator steering was also confirmed working in gameplay by a user; other mission-specific stick flows remain under investigation.

Earlier changes correct weapon pickup prompt spacing, add a button to the reload hint, and expand menu, vehicle and drone prompts without matching translated words.
