// Additive number-to-note mode. The original engine stays intact and handles
// every existing scale verbatim. Chromatic alone replaces the pitch mapping.
(() => {
  const original = {...module_engine};
  const mod = original.mod;
  const isChromatic = s => s.scale === 'Chromatic';
  const numberPitch = value => value === 0 ? null : mod(value - 1, 12);
  function validateComposition(c) {
    if (!isChromatic(c.settings)) return original.validateComposition(c);
    const check = original.validateComposition({...c, settings: {...c.settings,
      scale: 'Major', key: 0, chromatic: true, harmonicMode: 'scale'}});
    const errors = [...check.errors];
    for (const n of c.notes) {
      if (!Number.isSafeInteger(n.numberValue) || n.numberValue === 0 ||
          mod(n.midiNote, 12) !== numberPitch(n.numberValue)) {
        errors.push('Pitch does not match its input number');
      }
    }
    return {...check, valid: !errors.length, errors: [...new Set(errors)],
      checks: check.checks.map(x => x === 'Chromatic notes enabled' ?
        'Number mapping: 1=C through 12=B; 0=rest' : x)};
  }
  function compose(settings) {
    if (!isChromatic(settings)) return original.compose(settings);
    const chromaticSource = settings.chromaticSource || 'input';
    if (!['input','transformed'].includes(chromaticSource)) throw Error('Choose entered or transformed Chromatic numbers.');
    // Engines retain their timing, dynamics, form and register choices.
    // Fixed absolute pitch classes must not be quantized to a key or chord.
    const c = original.compose({...settings, scale: 'Major', key: 0,
      harmonicMode: 'scale', chromatic: false});
    c.settings = {...c.settings, scale: 'Chromatic', chromaticSource};
    const positions = Array(8).fill(0);
    const sectionPositions = c.sections.map(()=>Array(8).fill(0));
    const pitchSequences = c.sections.map((section,index)=> {
      // This mode has its own reproducible rotation, independent of timing RNG.
      const sequence = original.transform(c.sequence, section.transformation,
        new original.SeededRandom(settings.seed + ':chromatic:' + index));
      section.chromaticValues = sequence.slice(0,32);
      return sequence;
    });
    c.notes = c.notes.flatMap(n => {
      const sectionIndex = Math.max(0,c.sections.findIndex(s=>n.startBeat>=s.start-1e-6&&n.startBeat<s.end));
      const sequence = chromaticSource==='transformed' ? pitchSequences[sectionIndex] : c.sequence;
      const counters = chromaticSource==='transformed' ? sectionPositions[sectionIndex] : positions;
      const numberIndex = counters[n.track]++ % sequence.length;
      const numberValue = sequence[numberIndex];
      const pc = numberPitch(numberValue);
      if (pc === null) return [];
      const low = n.track === 1 ? 28 : n.track === 2 ? 40 : 48;
      const high = n.track === 1 ? 59 : n.track === 2 ? 84 : 96;
      let midiNote = 12 * Math.floor(n.midiNote / 12) + pc;
      while (midiNote < low) midiNote += 12;
      while (midiNote > high) midiNote -= 12;
      return [{...n, midiNote, numberValue, numberIndex, numberSection: sectionIndex}];
    });
    // Repeating values can now share a pitch. Keep complete, non-overlapping
    // note pairs on each original voice channel, as in the original exporter.
    c.notes.sort((a,b) => a.startBeat-b.startBeat || a.track-b.track || a.midiNote-b.midiNote);
    const last = new Map();
    for (const n of c.notes) {
      const key = n.track + ':' + n.midiNote, previous = last.get(key);
      if (previous && previous.startBeat + previous.durationBeats > n.startBeat)
        previous.durationBeats = Math.max(0, n.startBeat - previous.startBeat);
      last.set(key, n);
    }
    c.notes = c.notes.filter(n => n.durationBeats > 1/960/2);
    c.zeroMeaning = 'Zero is a rest. Each voice repeats the '+(chromaticSource==='transformed'?'section-transformed':'entered')+' numbers: 1=C through 12=B. Other nonzero integers wrap every 12 (13=C, -1=A#).';
    c.validation = validateComposition(c);
    return c;
  }
  function exportMidi(c) {
    if (!isChromatic(c.settings)) return original.exportMidi(c);
    const validation = validateComposition(c);
    if (!validation.valid) throw Error(validation.errors.join(', '));
    // Chromatic music has no diatonic key signature: use neutral 0 accidentals.
    return original.exportMidi({...c, validation,
      settings: {...c.settings, scale: 'Major', key: 0}});
  }
  Object.assign(module_engine, {SCALES: {...original.SCALES,
    Chromatic: Array.from({length:12},(_,i)=>i)}, compose, validateComposition, exportMidi});
})();
