const module_chords = (() => {
// Semitone intervals above the chord root. Screenshot labels retained verbatim.
const CHORDS={
'BLUES':[0,3,5,6,7,10],'MAJOR SCALE':[0,2,4,5,7,9,11],'MAJOR6':[0,4,7,9],'MAJOR7':[0,4,7,11],'MAJOR9':[0,4,7,11,14],'MINOR SCALE':[0,2,3,5,7,8,10],'MINOR/MAJOR7':[0,3,7,11],'MINOR6':[0,3,7,9],'MINOR7':[0,3,7,10],'MINOR9':[0,3,7,10,14],'MINOR7#5':[0,3,8,10],'MINOR7B9':[0,3,7,10,13],'MINOR/MAJOR9':[0,3,7,11,14],
'AUGMENTED':[0,4,8],'AUGMENTED9':[0,4,8,10,14],'AUGMENTEDMAJOR7':[0,4,8,11],'AUGMENTED7(7#5)':[0,4,8,10],'AUGMENTEDMAJOR9':[0,4,8,11,14],
'DIMINISHED':[0,3,6],'DIMINISHED7':[0,3,6,9],'DIMINISHED9':[0,3,6,9,14],'DIMINISHEDMINOR9':[0,3,6,9,13],'HALFDIMINISHED7(M7B5)':[0,3,6,10],'HALFDIMINISHED9':[0,3,6,10,14],'HALFDIMMINOR9':[0,3,6,10,13],
'DOMINANT':[0,4,7,10],'DOMINANT7':[0,4,7,10],'DOMINANT9':[0,4,7,10,14],'DOMINANT7B9':[0,4,7,10,13],'DOMINANT7#9':[0,4,7,10,15],'DOMINANT7#5B9':[0,4,8,10,13],
'ADD9':[0,4,7,14],'ADD11':[0,4,7,17],'6ADD9':[0,4,7,9,14],'7ADD11':[0,4,7,10,17],'7ADD13':[0,4,7,10,21],'MADD9':[0,3,7,14],'MADD11':[0,3,7,17],'M6ADD9':[0,3,7,9,14],'M7ADD11':[0,3,7,10,17],'M7ADD13':[0,3,7,10,21],'MAJ7ADD11':[0,4,7,11,17],'MMAJ7ADD11':[0,3,7,11,17],'MAJ7ADD13':[0,4,7,11,21],'MAJ7SUS4':[0,5,7,11],'MMAJ7ADD13#SUSPENDED':[0,3,6,7,11,21],
'SUS2':[0,2,7],'SUS4':[0,5,7],'6SUS4':[0,5,7,9],'7SUS4':[0,5,7,10],'7SUS4B9':[0,5,7,10,13],'9SUS4':[0,5,7,10,14],'MAJ9SUS4':[0,5,7,11,14],'CHROMATIC':[0,1,2,3,4,5,6,7,8,9,10,11]};
function chordDescription(name){const a=CHORDS[name];let mood=name.includes('SUS')?'Suspended, open tension.':name.includes('DIM')?'Tense, unstable color.':name.includes('AUG')?'Floating, unresolved color.':name.startsWith('MINOR')||/^M(?:ADD|6|7|MAJ)/.test(name)?'Minor-colored harmony.':name.startsWith('DOMINANT')?'Dominant tension with a pull toward resolution.':'Bright or expanded harmonic color.';if(name==='BLUES'||name.includes('SCALE')||name==='CHROMATIC')mood='A note collection used for melody and layered harmony.';let note=name==='MMAJ7ADD13#SUSPENDED'?' Nonstandard label: interpreted here as minor-major 7 + 13 + #11.':name==='DIMINISHEDMINOR9'||name==='HALFDIMMINOR9'?' “Minor9” is interpreted here as a flattened ninth.':name==='DOMINANT'?' Alias of DOMINANT7.':'';return `${mood} Semitones above the root: ${a.join(', ')}.${note}`;}

return {CHORDS, chordDescription};
})();