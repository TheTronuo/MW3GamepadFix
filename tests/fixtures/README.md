# Xbox camera numerical fixture

`console-aim-xbox.tsv` contains results from executing the original Xbox 360 PPC functions in Unicorn
2.1.4, with deterministic seed `360`. It contains numerical inputs and outputs, no game binary or
instruction bytes. The source Xbox `default.xex` SHA256 was:

```text
a78150499d0b5d08139b37006c87cb675bd86bc49ea52bec07bca880003e11d6
```

Rows cover 300 slowdown cases (`820C3F70`), 300 lock-on cases (`820C4AD0`), 100 angle-tracking cases
(`822E6B28`), 300 view-cone cases (`820C2878`), and 600 ADS frames (`820C43A0`) in 30 sequences.
The 30 `reset` rows delimit those sequences. `console_aim_tests.cpp` documents the input columns
through its parser and supplies the fixed target bounds, ranges and basis vectors.

The oracle supplies weapon ranges/class/type, profile values and the vector-to-angle helper using
controlled fixtures. ADS vectors use known axial/45-degree directions. PPC targeting, gates,
state updates, interpolation, slowdown, relative-velocity math and cone resistance execute from
the original image. Register-save helpers and the unsupported 64-bit stack save/load instructions
are handled by the harness; `floor` uses the host math helper. This is algorithm verification, not
an Xbox gameplay recording or proof that PC native target generation is identical.

`console-turn-xbox.tsv` adds 2,400 frames in 40 sequences (seed `36009`) from the original
curve (`820C3D88`, `8229F998`, `822E6BF8`) and turn/acceleration (`820C4088`, `822E69C8`)
instructions. The graph struct is populated with the seven extracted graph-3 points and unit
scale. Cone resistance is supplied as neutral and slowdown as controlled per-axis scales;
these are checked separately above. The turn receives each previously executed Xbox curve result.
Rows contain processed stick X/Y, seconds, ADS, FOV scale, pitch/yaw caps, pitch/yaw slowdown,
sensitivity, and expected pitch/yaw deltas. Tests preserve speed across frames between resets,
cover all four sensitivity presets, 30/60/120/100 Hz, partial inputs, release and reversal.
