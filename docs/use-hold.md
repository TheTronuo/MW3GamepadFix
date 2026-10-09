# Controller use hold

The controller hold policy delays five verified use scenarios until the engine's native use timer reaches 250 ms. Keyboard commands retain their native behavior. The mod does not change the global `g_useholdtime` value.

| Scenario | Native target | Localization key |
| --- | --- | --- |
| Swap for a ground weapon | Item, type 2, nonzero weapon | `PLATFORM_SWAPWEAPONS` |
| Pick up a new ground weapon | Item, type 2, nonzero weapon | `PLATFORM_PICKUPNEWWEAPON` |
| Recover a thrown throwing knife | Missile, type 3, weapon class 9 | `PLATFORM_PICKUPNEWWEAPON` |
| Pick up intelligence | Hinted entity, type 0 or 5 | `SCRIPT_INTELLIGENCE_PICKUP` |
| Start a breach | Hinted entity, type 0 or 5 | `SCRIPT_PLATFORM_BREACH_ACTIVATE` |

## Responsibilities

- `core/use_policy` owns the rule catalog, pure target classification and localized instruction conversion. Four rules cover five scenarios; ground weapon pickup and swapping share one native target route.
- `game/use_profile` owns executable addresses, verified function signatures and native field offsets for the supported build.
- `game/use_hold` reads a native target into a policy description and tracks command ownership. Scripted target keys come from the server configstring associated with the selected entity, rather than the client's current cursor text.
- `GameplayController` records the source of each generated command and asks the hold tracker whether to defer the existing timed entity-use call. `EngineApi` supplies verified native readers. The existing entity-use detour remains the single interception point.
- `PromptController` uses the same catalog for regular localization and direct HUD localization. `HudPromptCache` owns stable private entries; original database entries remain untouched.

## Timing and input ownership

The engine already maintains the selected entity handle and use start time. The mod reads those fields and checks that the current target matches the selected handle. It defers the timed callback while elapsed game time is below the rule's threshold, without restarting the timer or writing player/entity state.

Command source tags accompany the native command timestamp into server processing. A controller command queued before physical button release keeps its source; a keyboard-owned or unknown command follows the native PC path. The immediate entity-use call site is outside this policy. An inactive controller interaction is suppressed instead of completing a queued action.

Black Hawk boarding, generic doors, vehicles, turrets, NPC interactions and grenade throwback are outside this catalog. Script-owned input loops such as SDV mine planting are separate from the timed entity-use route.

## Localized prompts

Each rule names its eligible prompt keys and two original localized Press reference strings. Two native plant/defuse Hold strings provide the replacement instruction. Conversion compares the complete shared instruction on either side of the button slot, ignoring native color codes and surrounding ASCII whitespace. It preserves the target action text and button slot, and does not search for English or other translated words.

The audited weapon rules also permit an implicit instruction prefix. In the Russian PC archives, swap has no instruction before its button, while pickup has a nonempty prefix. When exactly one reference prefix is empty, either original prefix can become the shared native Hold prefix. This opt-in rule preserves both action suffixes and does not replace differing action prefixes in languages with post-button instructions.

Already-correct Hold text is preserved. A translation with missing, ambiguous or incompatible templates retains its original instruction; the hold behavior still applies. The test suite covers prefix and suffix instructions, multiple locales and markup variations. Actual language packs still require gameplay verification.

From 0.8.7, ground weapon and throwing knife hints additionally try the current locale's prone/crouch
Hold reference pair. When those templates are compatible, weapon swap and pickup use the complete
pickup action wording with the shorter Hold instruction. The original button slot is retained, and the
engine still appends the weapon name. Only the two weapon prompt keys participate; incompatible
translations retain the previous plant/defuse-based conversion. Other use scenarios keep their action text.

## Adding a scenario

1. Verify that it uses the timed entity-use route, and establish its native target type, discriminators, prompt key and console timing.
2. Add a `UseAction` and a catalog rule in `core/use_policy`, including its threshold, target conditions and localized reference keys. Reuse an existing action when the native route and policy are identical.
3. For the existing item, missile or hinted-entity routes, the native adapter and hooks need no new branch. A new target representation requires a verified reader in `game/use_hold` and build-specific offsets/signatures in `game/use_profile`.
4. Extend `use_hold_tests` with eligible/ineligible targets, timing boundaries and keyboard isolation. Add prompt template cases and HUD routing coverage as needed, then verify the scene in gameplay.

Keep scene-specific classification in the catalog and native adapter, rather than adding translated-word checks or separate timers to hooks.
