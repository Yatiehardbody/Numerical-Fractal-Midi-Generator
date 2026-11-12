import fs from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
const ctx={TextEncoder,structuredClone};vm.createContext(ctx);
vm.runInContext(fs.readFileSync(new URL('./packaged-core.js',import.meta.url),'utf8'),ctx);
const e=ctx.NMGReference.engine;
const post=vm.runInContext('module_postprocess',ctx);
const settings=(input,extra={})=>({...e.defaults(),scale:'Chromatic',key:7,
  mode:'sequence',input,seconds:30,voices:8,seed:'chromatic-1.1.2',...extra});
const pc=x=>((x%12)+12)%12;
// Parse actual exported SMF note-ons rather than only checking in-memory data.
function exportedPitches(bytes){
  const pitches=[];let pos=14;
  const u32=p=>((bytes[p]*0x1000000)+(bytes[p+1]<<16)+(bytes[p+2]<<8)+bytes[p+3]);
  while(pos<bytes.length){const end=pos+8+u32(pos+4);pos+=8;
    const vlq=()=>{let n=0,b;do{b=bytes[pos++];n=(n<<7)|(b&127);}while(b&128);return n;};
    while(pos<end){vlq();const status=bytes[pos++];
      if(status===255){pos++;const len=vlq();pos+=len;}
      else if(status===240||status===247){const len=vlq();pos+=len;}
      else{const kind=status&240;assert.ok(status>=128);const note=bytes[pos++];if(kind!==192&&kind!==208){const velocity=bytes[pos++];if(kind===144&&velocity)pitches.push(note);}}
    }assert.equal(pos,end);
  }return pitches;
}
for(let value=1;value<=12;value++){
  const c=e.compose(settings(String(value)));
  assert.ok(c.validation.valid,c.validation.errors.join());assert.ok(c.notes.length);
  assert.ok(c.notes.every(n=>n.midiNote%12===value-1));
  const pitches=exportedPitches(e.exportMidi(c));assert.equal(pitches.length,c.notes.length);
  assert.ok(pitches.every(n=>n%12===value-1));
}
for(const engine of e.ENGINES){
  const s=settings('1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0, 13, -1',{engine});
  const c=e.compose(s);assert.ok(c.notes.length);assert.ok(c.validation.valid);
  for(const n of c.notes){assert.notEqual(n.numberValue,0);assert.equal(n.numberValue,c.sequence[n.numberIndex]);assert.equal(pc(n.midiNote),pc(n.numberValue-1));}
  assert.deepEqual(e.exportMidi(c),e.exportMidi(e.compose(s)));
  const shifted=post.transposeComposition(c,1);assert.ok(shifted.validation.valid);assert.equal(shifted.notes.length,c.notes.length);
  for(let i=0;i<c.notes.length;i++)assert.equal(shifted.notes[i].midiNote,c.notes[i].midiNote+12);
  assert.ok(e.validateComposition(JSON.parse(JSON.stringify(shifted))).valid);
  const corrupted=structuredClone(c);corrupted.notes[0].midiNote++;
  assert.equal(e.validateComposition(corrupted).valid,false);
  assert.throws(()=>e.exportMidi(corrupted));
  const last=new Map();for(const n of c.notes){const k=n.track+':'+n.midiNote;assert.ok(!last.has(k)||last.get(k)<=n.startBeat+1e-8);last.set(k,n.startBeat+n.durationBeats);}
}
const zero=e.compose(settings('0'));assert.equal(zero.notes.length,0);assert.ok(zero.validation.valid);assert.equal(exportedPitches(e.exportMidi(zero)).length,0);
assert.ok(e.compose(settings('13')).notes.every(n=>n.midiNote%12===0));
assert.ok(e.compose(settings('-1')).notes.every(n=>n.midiNote%12===10));
const digit=e.compose(settings('128',{mode:'digit'}));assert.deepEqual(Array.from(digit.sequence),[1,2,8]);assert.ok(digit.notes.every(n=>[0,1,7].includes(n.midiNote%12)));
const chord=e.compose(settings('12',{harmonicMode:'chord',chord:'DIMINISHED',chromatic:true}));assert.ok(chord.notes.every(n=>n.midiNote%12===11));assert.equal(chord.settings.key,0);assert.equal(chord.settings.harmonicMode,'scale');
for(const [name,expected] of [['Original Sequence',[1,2,3]],['Reverse',[3,2,1]],['Cumulative Sum',[1,3,6]],['Differences',[-2,1,1]],['Mirror',[3,2,1]]]){
  const s=settings('1, 2, 3',{chromaticSource:'transformed',transforms:[name]});
  const c=e.compose(s);assert.ok(c.notes.length);assert.ok(c.validation.valid);
  for(const n of c.notes){assert.equal(n.numberValue,expected[n.numberIndex]);assert.equal(n.midiNote%12,pc(expected[n.numberIndex]-1));}
  assert.deepEqual(e.exportMidi(c),e.exportMidi(e.compose(s)));
  assert.ok(e.validateComposition(post.transposeComposition(c,-1)).valid);
  assert.ok(e.validateComposition(JSON.parse(JSON.stringify(c))).valid);
}
for(const transformation of e.TRANSFORMS){
  const c=e.compose(settings(Array.from({length:50},(_,i)=>i-10).join(', '),{chromaticSource:'transformed',transforms:[transformation]}));
  assert.ok(c.validation.valid);assert.ok(c.notes.every(n=>n.numberValue!==0&&n.midiNote%12===pc(n.numberValue-1)));
  assert.deepEqual(e.exportMidi(c),e.exportMidi(e.compose(c.settings)));
}
assert.throws(()=>e.compose(settings('1',{chromaticSource:'invalid'})));
console.log('PASS Chromatic: all 12 note mappings and exported MIDI, all engines, deterministic output, zero/rest, signed wrapping, digit parsing, key/chord independence, octave transposition, state roundtrip and note-pair safety.');
console.log('PASS transformed Chromatic: known Reverse/Cumulative/Differences/Mirror mappings, all transformations with 50-value sequences, deterministic export, state and transpose.');
