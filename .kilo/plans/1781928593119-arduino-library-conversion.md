# Plan: Convert rpdsp + audio into real Arduino libraries

Convert the canonical `dsp/` and `audio/` trees into two bundled Arduino
libraries so the ~100 hand-synced per-example copies can be deleted and the
"propagate every change to every example" rule retired.

## Resolved decisions

1. **Packaging = Model A (two bundled libraries).** `libraries/rpdsp` (MIT,
   ours, header-only) and `libraries/pico_audio_i2s` (BSD-3, vendored from
   pico-extras). Both coexist in this repo. License/ownership boundary stays
   clean and matches the existing dsp/audio split.
2. **Namespaced includes.** Public paths are `<rpdsp/...>` and
   `<pico_audio_i2s/...>`. Internal headers keep their quote-form sibling
   includes (`#include "algorithm.h"` etc.) — these resolve relative to the
   including file, so **zero internal include edits** are needed.
3. **Install = docs + `arduino-cli --library` flag.** Per-example
   self-containment is an accepted tradeoff (examples now require both
   libraries on the include path).

## De-risked facts (verified)

- `audio_i2s.pio.h` is self-contained pioasm output; `audio_i2s.c` includes
  the generated header. **No PIO build step is required** in the library.
- `SuperSaw.h` is self-contained (`namespace rpdsp`, bare sibling includes).
- All internal includes are quote-form relative names.

## Target layout

```
Pico-DSP-Garden/
  libraries/
    rpdsp/
      library.properties            # name=rpdsp, MIT, architectures=rp2040
      src/rpdsp/                    # = current dsp/* (16 headers) + hypersaw.h
        algorithm.h config.h delay_line.h dynamics.h effects.h envelope.h
        filter.h gate_pattern.h hypersaw.h joystick_recorder.h knob_bank.h
        oscillator.h parameter_smoother.h pickup_knob.h realtime.h
        rhythm_sequencer.h voice.h
      extras/audio_i2s.pio          # see Risk #1 (PIO source archive)
    pico_audio_i2s/
      library.properties            # name=Pico Audio I2S, BSD-3, architectures=rp2040
      src/pico_audio_i2s/           # = current audio/* (9 files, incl. generated .pio.h)
        audio.h audio.cpp buffer.h buffer.c audio_i2s.h audio_i2s.c
        audio_i2s.pio.h sample_conversion.h
  Examples/
    SuperSaw/SuperSaw.ino           # includes rewritten; src/ deleted
    Oscillators/Oscillators.ino
    SimpleOscillators/SimpleOscillators.ino
    TwoChannelOscillator/TwoChannelOscillator.ino
  Docs/  README.md  AGENTS.md  .gitignore   # updated
  # REMOVED: dsp/  audio/  Examples/*/src/
```

## library.properties specs

`libraries/rpdsp/library.properties`:
```
name=rpdsp
version=0.1.0
author=IC-Alchemy
maintainer=IC-Alchemy
sentence=Header-only real-time audio DSP modules for embedded use.
paragraph=Band-limited oscillators, state-variable filters, dynamics, Schroeder reverb, envelopes, sequencing/UI helpers. Portable C++17.
category=Signal Processing
url=https://github.com/IC-Alchemy/Pico-DSP-Garden
architectures=rp2040
```

`libraries/pico_audio_i2s/library.properties`:
```
name=Pico Audio I2S
version=0.1.0
author=Raspberry Pi (Trading) Ltd. (pico-extras), packaged by IC-Alchemy
maintainer=IC-Alchemy
sentence=PIO/DMA I2S audio output driver for RP2040/RP2350.
paragraph=Producer/consumer audio buffer pools and I2S transport via PIO. Vendored from Raspberry Pi pico-extras. BSD-3-Clause.
category=Signal Input/Output
url=https://github.com/IC-Alchemy/Pico-DSP-Garden
architectures=rp2040
```

Note: folder name is the include-path root. Folder `pico_audio_i2s` →
`<pico_audio_i2s/audio.h>`; the `name` field is only the display name.

## Ordered task list

Do these in order so the tree stays compilable at each step. Use `git mv` to
preserve history; stage the whole change as one reviewable unit.

**Step 1 — Scaffold libraries.**
- Create `libraries/rpdsp/` and `libraries/pico_audio_i2s/` with the
  `library.properties` files above and empty `src/<lib>/` dirs.

**Step 2 — Move canonical trees in (git mv).**
- `git mv dsp/*.h libraries/rpdsp/src/rpdsp/`
- `git mv audio/* libraries/pico_audio_i2s/src/pico_audio_i2s/`
- Move the PIO source out of the compiled src to avoid the PIO build recipe:
  `git mv libraries/pico_audio_i2s/src/pico_audio_i2s/audio_i2s.pio
  libraries/pico_audio_i2s/extras/audio_i2s.pio` (keep the generated
  `audio_i2s.pio.h` in src; see Risk #1).
- Remove now-empty `dsp/` and `audio/` root dirs.

**Step 3 — Promote Hypersaw into rpdsp.**
- `git mv Examples/SuperSaw/src/dsp/SuperSaw.h
  libraries/rpdsp/src/rpdsp/hypersaw.h` (rename to lowercase to match rpdsp
  file convention; class stays `rpdsp::Hypersaw`). Its internal includes
  (`"algorithm.h"` etc.) are siblings and need no change.

**Step 4 — Rewrite example includes.**
In each of the 4 `.ino` files, replace:
- `#include "src/audio/audio.h"`        → `#include <pico_audio_i2s/audio.h>`
- `#include "src/audio/audio_i2s.h"`    → `#include <pico_audio_i2s/audio_i2s.h>`
- `#include "src/dsp/oscillator.h"`     → `#include <rpdsp/oscillator.h>`
- `#include "src/dsp/SuperSaw.h"`       → `#include <rpdsp/hypersaw.h>`  (SuperSaw.ino only)
Leave every other line (pins, SAMPLE_RATE, DSP logic) untouched.

**Step 5 — Delete the duplicated copies.**
- `git rm -r Examples/SuperSaw/src Examples/Oscillators/src
  Examples/SimpleOscillators/src Examples/TwoChannelOscillator/src`
- Removes ~100 tracked duplicate files.

**Step 6 — Update docs/config.**
- `README.md`: rewrite "Repository layout" to show `libraries/`; in
  "Getting started" add the install step (copy or symlink the two
  `libraries/*` into the Arduino sketchbook `libraries/` folder, or build with
  `arduino-cli --library`). While here, fix the stale "DaisySP" references →
  rpdsp (README currently still claims DaisySP).
- `AGENTS.md`: retire the "canonical-copy rule" section — libraries are now the
  single source of truth, examples depend on them via `<rpdsp/...>` /
  `<pico_audio_i2s/...>`, build with `--library`. Update the
  `Examples/SuperSaw/src/dsp/SuperSaw.h` note (now
  `libraries/rpdsp/src/rpdsp/hypersaw.h`). Keep the rpdsp API conventions,
  pin table, sample-rate inconsistency, and dual-core notes.
- `.gitignore`: add `Examples/*/build/` and `libraries/*/build/` (scoped only;
  the broader broken-.gitignore rewrite is the separate hygiene workstream).

## Validation

**A. Host syntax check (no hardware, fast) — rpdsp only (portable, header-only):**
```
# From repo root. Aggregator TU over every public rpdsp header.
g++ -std=c++17 -fsyntax-only -Ilibraries/rpdsp/src -xc++ - <<'EOF'
#include <rpdsp/algorithm.h>
#include <rpdsp/config.h>
#include <rpdsp/delay_line.h>
#include <rpdsp/dynamics.h>
#include <rpdsp/effects.h>
#include <rpdsp/envelope.h>
#include <rpdsp/filter.h>
#include <rpdsp/gate_pattern.h>
#include <rpdsp/hypersaw.h>
#include <rpdsp/joystick_recorder.h>
#include <rpdsp/knob_bank.h>
#include <rpdsp/oscillator.h>
#include <rpdsp/parameter_smoother.h>
#include <rpdsp/pickup_knob.h>
#include <rpdsp/realtime.h>
#include <rpdsp/rhythm_sequencer.h>
#include <rpdsp/voice.h>
int main(){return 0;}
EOF
```
Any failure here = a namespaced-move include breakage. Fix before proceeding.

**B. Firmware compile of each example (target: `rp2040:rp2040:rpipico2`):**
```
arduino-cli compile \
  --fqbn "rp2040:rp2040:rpipico2:flash=4194304_0,freq=200,opt=Optimize3,usbstack=picosdk" \
  --library libraries/rpdsp --library libraries/pico_audio_i2s \
  Examples/<Name>
```
Run for: SuperSaw, Oscillators, SimpleOscillators, TwoChannelOscillator.
(No host build, test framework, or CI exists in this repo per AGENTS.md;
verification is "compile each example, flash, listen.")

## Risks & fallbacks

1. **arduino-pico PIO recipe may try to compile `audio_i2s.pio` inside the
   library** and collide with the committed `audio_i2s.pio.h`. **Default
   mitigation (already in Step 2):** keep `.pio` in `extras/` (Arduino ignores
   it) and ship only the generated `.pio.h`. If a build still complains,
   confirm `audio_i2s.c` resolves `"audio_i2s.pio.h"` to the committed sibling.
2. **Include-path breakage from the namespaced move.** Caught immediately by
   validation A (rpdsp) and B (audio, via firmware compile). Quote-form sibling
   resolution means internal includes should be untouched; if any fail, add the
   missing sibling include explicitly.
3. **`architectures=rp2040` hides rpdsp on non-pico boards.** Intended (scoped
   to this firmware). If host-portability of the lib as a standalone install is
   later wanted, change rpdsp to `architectures=*`.
4. **Committed `build/` (SuperSaw's) and `.vscode/` reference the old
   structure.** They are stale artifacts of the separate hygiene workstream;
   not blocking, but expect them to be regenerated/removed there.
5. **Windows path separators in `arduino-cli --library`.** Use forward-slash
   relative paths as shown; arduino-cli accepts them on Windows.

## Out of scope (separate workstreams)

- Untracking committed `build/`, `.vscode/`, `.qoder/` and rewriting the broken
  `.gitignore` (P0 repo hygiene).
- Reconciling docs with reality (phantom `DefaultAudioBlock`/`MidiByteParser`/
  `CallbackPerformanceMeter` API; 48k-vs-44.1k / 32-vs-256 block-size claims).
- Adding the missing `LICENSE` file (MIT) — recommend doing this first.
- Extracting shared I2S/int16 scaffolding into the audio library; sample-rate
  normalization across examples.
- Arduino Library Manager publishing (not chosen; Model A = bundled).

## Open questions for the user (minor, non-blocking)

- Confirm `author`/`maintainer` = "IC-Alchemy" in both `library.properties`
  (inferred from README's GitHub URL); adjust if different.
- Confirm renaming the promoted file to `hypersaw.h` (vs keeping
  `SuperSaw.h`). Plan assumes lowercase per rpdsp convention.
