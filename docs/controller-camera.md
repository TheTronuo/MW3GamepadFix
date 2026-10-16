# Controller camera

Version 0.8.9 ports the missing default Xbox 360 camera stages into the supported PC executable.
It uses native PC weapon data, visible screen targets, camera state and frame durations. Game files
on disk are not patched. Keyboard input and inactive controller calls retain the native aim path.

## Processing order

The native command builder finishes keyboard movement before `MouseMove` (PC RVA `7D9F0`).
The mod adds controller movement here and scopes the current camera command. The native wrapper
at `3FC0` prepares screen metrics, ADS/FOV and the previous button state around its call to `2FE0`.
That call is detoured for the active controller command:

1. Xbox response curve and view-cone resistance.
2. Native weapon ranges, target/script slowdown, sensitivity, FOV, turn caps and acceleration.
3. ADS edge acquisition and angle tracking.
4. Melee acquisition followed by native PC melee charge/tag lookup/interpolation.
5. Relative target/player velocity lock-on, then native packing of final angles and charge data.

Target lists retain the native visibility filtering and sort order. The previous lock target has
priority within its range/box. Lock-on examines yaw, pitch and strafe axes; forward movement alone
does not engage it. ADS acquisition happens on the button edge, not repeatedly while held. Native
weapon transitions, offhand use and melee phase gates are preserved. Pause, freeze, disconnect,
remote camera and invalid frame durations reset the mod's camera/aim state.

## Binary correspondence

Xbox addresses refer to the inspected decrypted `default.xex` image. PC addresses are RVAs in the
supported hash listed in [verification](verification.md).

| Stage/data | Xbox 360 | PC port |
| :--- | :--- | :--- |
| Aim pipeline | `820C4CF8` | Wrapper `3FC0`, detour/trampoline `2FE0` |
| Response / turn | `820C3D88` / `820C4088` | `StickCamera` |
| View-cone resistance | `820C2878` | `console_view_resistance`, cached player state at `6E26E0 + 2088` |
| Target/script slowdown | `820C3F70` | `console_slowdown` |
| ADS acquisition / tracking | `820C43A0` / `822E6B28` | `ConsoleAim::auto_aim` / `console_track_angle` |
| Lock-on | `820C4AD0` | `ConsoleAim::lock_on` |
| Melee acquisition | `820C47A0` | Expanded region/prior target, original PC `2FE0` for charge processing |
| Camera state | `82515F90`, stride `E80` | `4D9A00`, stride `E60` |
| Weapon definitions | Xbox 32-bit definitions | Definition table `4E8E60`, type `54`, ranges `520/524/528` |
| Attachments / class | `8235A788` / `8235F798` and related helpers | Native helpers `26B50` / `26D40` |

The PC camera target list starts at `134`, count at `E34`, stride `34`. Each entry contains entity,
screen bounds, position, velocity and squared distances. PC camera state stores script slowdown at
`E38/E3C/E40`, melee phase/target at `E44/E48`, ADS at `120`, FOV at `118`, eye at `DC`, angles at
`E8`, right/up vectors at `100/10C`. Cached player-state view-cone fields are unchanged between
these builds: angles `10C`, centers `12C`, ranges `134`, negative/positive zones `13C/144`.

Sensitivity presets use the Xbox configuration multipliers: low `0.6`, medium `1.0`, high `1.8`,
ultra `4.0`. Target slowdown scales are `0.4` at hip and `0.5` at full ADS. The lock-on coefficient
is `0.6`, the stick activation threshold `0.05`, and ADS angle tracking uses `40/s`. No additional
moving average or smoothing tail is added.

Target assistance is disabled by default (`aim_assist=0`), including when upgrading with an older
INI that has no such key. Set `aim_assist=1` and restart to enable slowdown, ADS assistance and
tracking. Scripted slowdown, turn caps and view-cone resistance remain active in either mode.

The static camera and predicted-player ranges are checked per call before their fields are read.
Dynamic weapon and attachment pointers retain separate checks. This avoids a memory-query syscall
for every field and target without keeping a cache of memory protections across frames.

## Verification scope

The checked-in numerical fixture is described in [fixture provenance](../tests/fixtures/README.md).
It verifies the pure camera calculations and ADS state transitions against original PPC execution.
The executable hash guard and hook relocation test cover installation on the supported PC build.
The PC native target generation and remaining melee processing are not executed by the numerical
fixture. Full in-game comparisons across missions, weapons, frame rates and vehicles remain pending.
Alternate stick layouts and location-selector controls are outside this camera change.
