import fs from 'node:fs';import vm from 'node:vm';import assert from 'node:assert/strict';import crypto from 'node:crypto';
const load=file=>{const c={TextEncoder,structuredClone};vm.createContext(c);vm.runInContext(fs.readFileSync(file,'utf8'),c);return c.NMGReference.engine;};
const original=load(new URL('../Reference/core.js',import.meta.url)),packaged=load(new URL('./packaged-core.js',import.meta.url));const fixtures=JSON.parse(fs.readFileSync(new URL('../Reference/fixtures.json',import.meta.url)));
const hash=x=>crypto.createHash('sha256').update(x).digest('hex');for(const f of fixtures){const c=packaged.compose(f.settings);assert.equal(hash(packaged.exportMidi(c)),f.midiSHA256);assert.equal(hash(JSON.stringify(c.notes)),f.eventsSHA256);assert.equal(JSON.stringify(c.notes),JSON.stringify(original.compose(f.settings).notes));}
const html=fs.readFileSync(new URL('../Web/index.html',import.meta.url),'utf8');for(const name of ['engine','chords'])assert.ok(html.includes(fs.readFileSync(new URL(`../Reference/${name}.js`,import.meta.url),'utf8')));
assert.equal(packaged.defaults().voices,5);assert.ok(html.includes('channels:[1,1,1,1,1,1,1,1]'));for(const role of ['Lead','Bass','Harmony','Counterpoint','Additional Melody','Mathematical Texture','Canon Voice','Upper Texture'])assert.ok(html.includes('('+role+')'));
console.log('PASS packaged engine: 78 byte-identical MIDI and event fixtures; exact original engine/chords, voice defaults and labels.');

const fresh=packaged.defaults();assert.equal(fresh.preset,'Fractal');assert.equal(fresh.engine,'AUTO');assert.deepEqual(JSON.parse(JSON.stringify(fresh.influence)),JSON.parse(JSON.stringify(original.presetSettings('Fractal',original.defaults()).influence)));
console.log('PASS initial Fractal profile / AUTO engine, with original explicit-setting fixture outputs preserved.');
