# Compatibility and verification

The mod targets the Windows x64 single-player executable `iw5sp.exe` from Call of Duty: Modern Warfare 3 (2011).

Supported executable SHA256:

```text
a97d2bbc7e495e4cf1a3b7e9a021e25b2c7b461d63f2b88d3cfe8782e8dc1023
```

The executable hash and hook signatures are checked before hooks are enabled. Other executable builds and multiplayer are unsupported. The mod does not patch game files on disk.

## Version 0.8.8

The Warlord mortar entry hint now uses the current locale's Press instruction instead of Hold.
The script registers one button hint (`WARLORD_HINT_USE_MORTAR`) on the usable mortar and reuses it
after dismount. Entry follows its trigger immediately; exit waits for release then a fresh Use press;
fire responds to the attack command. Reloading is part of the firing animation. No other button hint
was found in this mortar script, or other initializations of this mortar library in the inspected campaign.

Two native Press references and two native Hold references identify the complete instruction before
or after the button slot. The mortar action, punctuation, encoding and `&&1` parameter are preserved.
Both regular localization and the direct HUD lookup apply the same narrowly scoped conversion.
Missing/incompatible templates and already-correct Press text retain the original string. Weapon
pickup, prone and the timed Use catalog retain their policies. Input behavior itself is unchanged.
All 17 Release CTest tests pass. Mortar cases cover the Russian retail strings, synthetic English,
Arabic and Japanese-style templates, prefix/suffix conversion, caller and key isolation, parameter
retention, idempotence, cache lifetime and fallback behavior. Gameplay verification of the revised
hint and other language packs is pending.

## Version 0.8.7

Controller weapon swap and pickup prompts share the current locale's weapon pickup action text.
Compatible native prone/crouch Hold templates supply the shorter instruction. In the installed Russian
resources this produces `Удерживайте [button], чтобы взять`, followed by the engine's weapon name.
The original native button parameter, appended weapon name, 250 ms timing and keyboard path are retained.
Intelligence, breach and other use prompts keep their existing wording. Incompatible or missing compact
templates fall back to the previous localized Hold policy. No translations are hardcoded or matched by word.

All 17 Release CTest tests pass. The policy and direct HUD cache tests cover actual Russian strings, English, synthetic prefix/suffix
translations, missing templates, unrelated keys, repeated conversion and native parameter retention.
This wording is pending gameplay verification; other installed language packs have not been tested.

## Version 0.8.6

Controller prone instructions use the complete current-locale `SCRIPT_PLATFORM_HINT_HOLDDOWNPRONEKEY`
translation. The ten audited keys cover Castle, Prague, all four Warlord binding variants and four shared
prone templates. Warlord's four variants describe one scene; the campaign script review identified six
display contexts across the three missions. Subtitles, crouch, stand-up instructions, blocked messages,
menu labels and death quotes are excluded.

Both regular localization and the formatter's existing direct localization lookup use this policy.
Mission instructions receive the stance button icon directly. Parameterized instructions retain `&&1`
so native formatting still consumes the caller's button argument. No translated words are searched or
spliced; text order, encoding and colors come from the native Hold string. Missing or malformed templates
retain the original instruction. Keyboard mode and disabled gameplay use the original entries.

All 17 Release CTest tests pass. The HUD tests cover all ten keys, input/caller isolation, native argument retention, byte preservation
for Russian CP1251, English, Arabic and a Japanese-style translation, repeated conversion, unrelated
messages and pointer lifetime across locale changes. Only the installed Russian and previously inventoried
English native strings have been inspected; the other test strings are synthetic. Gameplay and other
language packs still need verification. This build remains a pre-release.

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
