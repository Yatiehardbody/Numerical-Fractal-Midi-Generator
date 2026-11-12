# Chromatic number mapping — NMG 1.1.2

Choose Chromatic under Scale / Mode. The mapping is absolute:

| Number | Note |
|---|---|
| 1 | C |
| 2 | C# |
| 3 | D |
| 4 | D# |
| 5 | E |
| 6 | F |
| 7 | F# |
| 8 | G |
| 9 | G# |
| 10 | A |
| 11 | A# |
| 12 | B |

The existing Digit Mode / Sequence Mode buttons still determine how input is read. Digit Mode reads 128 as 1, 2, 8. Sequence Mode reads 12, 8 as two whole values, B and G.

The new Pitch numbers selector offers two choices:

- Entered numbers: each voice cycles through the parsed input.
- Transformed numbers: each voice cycles through the selected section transformation, restarting at each section. Reverse reverses the values; Cumulative Sum adds them; the other transformation choices work similarly. Rotate uses a separate reproducible seed for the Chromatic pitch sequence.

Zero creates a rest. Nonzero integers wrap every 12: 13=C, 24=B, -1=A#. In transformed mode, this rule applies to the transformed value. For example, Cumulative Sum of 1, 2, 3 is 1, 3, 6, producing C, D, F.

Key, chord and occasional accidental controls are disabled while Chromatic is selected, so C always remains 1. Engines, numerical influence and transformations still control the timing, dynamics, activity, section structure and octave register. They can create rests and simultaneous notes, so this is a generative composition rather than a plain monophonic sequence player. Same-pitch overlaps within a voice are resolved before export.

Octave transposition, MIDI drag/export, live channel routing and preset/project saving remain available. The new selector is saved in the composition settings. Every existing non-Chromatic mode retains its original generation behavior.
