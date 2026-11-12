# NMG 1.1.4 — Numerical MIDI Generator

Native Windows x64 VST3 and Apple Silicon macOS VST3/AU plugins built from the supplied Numerical MIDI Generator HTML. The original JavaScript mathematical engine and original MIDI exporter remain verbatim. No standalone application is included.

Start with **docs/INSTALLATION.md** and **docs/FL-STUDIO-ROUTING.md**. Test coverage and remaining limitations are in **docs/TEST-RESULTS.md** and **PROGRESS.md**.

All eight voice routes initially use MIDI Channel 1; five voices are active by default. Expand **Voice → MIDI channel assignments** to choose each channel independently. Inactive voices retain their assignments. Changes affect live MIDI only, after generation.

## Source layout

- `Reference/Original.html`: unmodified authoritative source, including embedded preview samples.
- `Reference/engine.js`, `chords.js`: exact extracted mathematical modules.
- `Reference/fixtures.json`: original deterministic reference outputs.
- `Web/index.html`: embedded application generated from that source.
- `Web/bridge.js`: interface/host adapter, separate from the engine.
- `Source/`: JUCE plugin and native MIDI scheduler.
- `Tests/`: equivalence, scheduler, processor and automated editor checks.
- `tools/`: source extraction, interface generation and build scripts.
- `vendor/JUCE/`: JUCE 8.0.12 source and dependency licence notices.

`Original MIDI` exports the authoritative original Type 1 MIDI with its original per-voice channels and program changes. The voice routing controls affect live host MIDI, which sends no program changes. This distinction preserves byte-for-byte original export compatibility.

Internal preview auditions through system audio while the editor is open. External MIDI is the mode for DAW instruments, host synchronization and recording/rendering. NMG's stereo output is silent; it does not render the browser preview into the DAW mixer.

This package contains development builds. Read the actual test results before treating an untested DAW workflow as verified.

## Version 1.1 update

Remembers resized windows; moves playback destination/scale/sync into the header; adds a saved color theme picker and native MIDI drag-out; lists octave transpositions high-to-low; removes preview volume control and uses 100%; includes a Mac uninstaller. See docs/INSTALLATION.md for upgrade, drag and uninstall instructions.


Version 1.1.1 adds a visible version badge, a version-specific embedded page, a close-FL-Studio installer requirement and independent compiled-VST3 tests. Fully quit FL Studio before updating; closing only the plugin window can leave the previous binary loaded.


## Version 1.1.2: Chromatic number mapping

Select **Chromatic** under **Scale / Mode**. Choose **Entered numbers** to repeat the input, or **Transformed numbers** to map each section’s transformed sequence. Both use absolute pitch classes: 1=C, 2=C#, 3=D, 4=D#, 5=E, 6=F, 7=F#, 8=G, 9=G#, 10=A, 11=A#, 12=B. Zero is a rest; nonzero signed integers wrap every 12 (13=C, -1=A#). Select Sequence Mode to enter 10, 11 and 12 as whole values; Digit Mode still splits each digit.

Chromatic ignores tonic/chord and accidental settings so the mapping remains fixed. Engines and transformations still shape rhythm, density, dynamics, form and octave register. In transformed mode, each voice restarts the transformed sequence at each section, with a separate reproducible rotation seed. Each generated voice advances independently through the selected sequence; it may skip silent slots or have fewer simultaneous notes after overlap cleanup. Octave transposition remains available after generation.

The additive adapter is Web/chromatic.js. Reference/Original.html and its engine/chord modules are unchanged; all 78 existing deterministic cases still match byte for byte. The new mode has its own mapping/export/transposition tests in Tests/chromatic.mjs.


## Version 1.1.3: defaults, piano-roll theme and smooth preview

New instances start with the #A30000 theme, the full Fractal palette profile and AUTO composition engine. Reset theme restores this red. Explicit colors and compositions saved in existing presets/projects are retained. Piano-roll background, grid, section lines and labels follow theme changes immediately; voice colors stay distinct.

Original Synth preview now uses continuous attack/decay/sustain/release ramps and fades active sources on pause, stop, seek and instrument changes. The previous abrupt 28% gain drop and hard source cutoffs are removed. Sample preview also fades on interruption. The preview master level remains at its previous maximum setting. This changes preview audio only; generated MIDI notes and velocities are untouched.


## Version 1.1.4: green theme and internal preview default

New instances default to #3EE23E and Internal preview · system audio. Theme reset returns to this green. Fractal + AUTO, Chromatic modes and smooth preview audio remain available. Saved presets/projects retain explicit colors and playback destinations. Both Mac VST3 and AU are updated.
