# Compatibility and verification

The mod targets the Windows x64 single-player executable `iw5sp.exe` from Call of Duty: Modern Warfare 3 (2011).

Supported executable SHA256:

```text
a97d2bbc7e495e4cf1a3b7e9a021e25b2c7b461d63f2b88d3cfe8782e8dc1023
```

The executable hash and hook signatures are checked before hooks are enabled. Other executable builds and multiplayer are unsupported. The mod does not patch game files on disk.

## Version 0.8.2

The Release build passes all 14 CTest tests, covering input handling, menus, gameplay actions, controller prompts, camera hooks, hook lifetime and ASI startup.

Prompt coverage was reviewed against 93 installed single-player archives. The current catalog handles 63 prompt contexts and all 30 explicit action tokens identified by that review. Tests exercise English, Russian and Arabic text preservation, both button styles, repeated substitutions, and inline material records.

This archive review and the automated tests do not establish that every campaign scene works in gameplay. Full campaign, vehicle, mission-specific and Special Ops verification is ongoing; this release is marked as a pre-release.

Recent changes correct weapon pickup prompt spacing, add a button to the reload hint, and expand menu, vehicle and drone prompts without matching translated words.
