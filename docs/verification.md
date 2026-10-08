# Compatibility and verification

The mod targets the Windows x64 single-player executable `iw5sp.exe` from Call of Duty: Modern Warfare 3 (2011).

Supported executable SHA256:

```text
a97d2bbc7e495e4cf1a3b7e9a021e25b2c7b461d63f2b88d3cfe8782e8dc1023
```

The executable hash and hook signatures are checked before hooks are enabled. Other executable builds and multiplayer are unsupported. The mod does not patch game files on disk.

## Version 0.8.5

Controller use now waits for 250 ms of native game time for ground weapon swapping, new weapon pickup, recovery of a thrown throwing knife, intelligence pickup and breach activation. This matches the inspected Xbox `g_useholdtime` default. Four rules cover the five scenarios because the two ground weapon actions share the native item route. The same catalog controls action eligibility and localized hold prompts; see [the architecture and extension guide](use-hold.md).

The detour is restricted to the timed entity-use call site, the selected native use target and a controller-owned `+usereload` command. Scripted targets are identified by their server configstring localization keys; thrown knives also require the native weapon class. It retains the engine's use handle and start timestamp while waiting. The shared `g_useholdtime` setting, keyboard use, reload, grenade throwback and other interactions remain unchanged.

Command source tags follow the native usercmd into server processing, including a command queued before the physical X button was released. The supported PC entity-use prologue, weapon-class and server-configstring reader signatures are checked against the executable. Entity-use trampoline relocation is tested. All 17 Release CTest tests pass, including the five scenarios, the 249/250 ms boundary, source isolation, release, target selection, configstring bounds, changed cursor hints, clock wrap/reset, inactive gameplay and unchanged native memory.

Eligible prompt keys reuse the current locale's plant/defuse hold templates and the original action text. Both the regular localization path and the HUD formatter's direct asset lookup use the same conversion. Only a complete shared instruction before or after the button slot is transferred; translated words are never searched. Tests cover English, Russian, Arabic and a Japanese-style post-button instruction, differing color/spacing markup, already-localized hold text, and stable private HUD entries. Missing or incompatible translation templates preserve the original text.

Inspection of the installed Russian archives showed that `PLATFORM_SWAPWEAPONS` has an empty instruction prefix and `PLATFORM_PICKUPNEWWEAPON` has a nonempty one. The weapon rules explicitly support that mixed pair using the native localized Hold prefix. Regression tests include the actual Russian weapon and script strings, HUD entry conversion, opt-in boundaries and suffix-instruction preservation.

Ground weapon swapping with holding X and the corrected Russian Hold hint were confirmed working in gameplay by a user on October 9, 2026. The remaining scenarios and other language packs still need gameplay verification. This version remains a pre-release.

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
