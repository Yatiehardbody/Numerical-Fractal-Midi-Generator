from pathlib import Path
import re, colorsys, json
root=Path(__file__).resolve().parents[1];version=re.search(r'project\(NMG VERSION ([0-9.]+)',(root/'CMakeLists.txt').read_text(encoding='utf-8')).group(1);html=(root/'Reference/Original.html').read_text(encoding='utf-8');bridge=(root/'Web/bridge.js').read_text(encoding='utf-8')
# Preserve source engine/chords byte for byte; only adapter and UI sections are changed.
script=re.search(r'<script>\s*(.*?)</script>',html,re.S).group(1)
app_start=script.index('const module_app =')
core,app=script[:app_start],script[app_start:]
app=app.replace('const v=i+range.min;', 'const v=range.max-i;').replace("v===0?'Original pitch'","v===0?'0 Octaves'")
app=app.replace("$('volume').oninput=e=>audio.setVolume(+e.target.value/100);",'')
app=app.replace('const audio=new AudioPreview(', 'const audio=new AudioPreview(')
app=app.replace('${t.name}</span>','${NMG_VOICE_LABELS[TRACKS.indexOf(t)]}</span>')
app=app.replace("drawRoll();renderSection();}","drawRoll();renderSection();nmgBridge.voiceActivity(s.voices);nmgBridge.publish(c,read());}")
a=app.index('function download(data,name,type){');b=app.index("\n$('transpose')",a)
app=app[:a]+"function download(data,name,type){nmgBridge.file(data,name,type);}"+app[b:]
# Explicit project/preset persistence instead of shared browser localStorage.
app=app.replace("try{const saved=JSON.parse(localStorage.getItem('numerical-settings-v1'));if(saved){compose(saved);settings={...defaults(),...saved};}}catch{}",'')
app=app.replace("function markDirty(){if(composition)","function markDirty(){if(composition)nmgBridge.publish(composition,read());if(composition)")
# Original functions are still used for generation and restored composition validation.
end='installHelp();fill();generation();'
assert end in app
app=app.replace(end,end+'''
audio.setVolume(1);
nmgBridge.setup({midi:()=>exportMidi(composition),midiName:()=>downloadName(composition,'midi'),notice:toast,stopPreview:()=>audio.stop(),initial:()=>{settings=defaults();fill();generation();},presetName:()=>downloadName(composition,'settings'),legacy:s=>{settings={...defaults(),...s};fill();generation();},restore:state=>{
 settings={...defaults(),...state.settings};const restored=structuredClone(state.composition);if(!restored?.notes||restored.notes.length>500000)throw Error('Invalid composition');const check=module_engine.validateComposition(restored);if(!check.valid)throw Error(check.errors.join(', '));
 const shift=Number(restored.settings.transposeOctaves)||0;sourceComposition={...restored,settings:{...restored.settings,transposeOctaves:0},notes:restored.notes.map(n=>({...n,midiNote:n.midiNote-12*shift}))};composition=restored;audio.setComposition(composition);fill();const range=octaveRange(sourceComposition);$('transpose').innerHTML=Array.from({length:range.max-range.min+1},(_,i)=>{const v=range.max-i;return `<option value="${v}">${v>0?'+':''}${v} Octaves</option>`;}).join('');$('transpose').value=shift;selectedSection=0;render();$('dirty').hidden=true;
 },position:beat=>{if(!composition)return;const seconds=Math.max(0,Math.min(composition.settings.seconds,beat*60/composition.settings.bpm));$('currentTime').textContent=time(seconds);$('timeline').value=seconds;$('playhead').style.left=(38+($('rollInner').clientWidth-38)*seconds/composition.settings.seconds)+'px';}});
''')
core=core.replace("function textFor(el){", "function textFor(el){if(el.id==='dragMidi')return 'Drag the original multitrack MIDI file into your DAW. Uses the same bytes and original channels as Original MIDI.';if(el.id==='nmgTheme')return 'Choose a theme using hue, saturation, brightness, RGB or a hex color. Saved with your preset and project.';if(el.dataset.voice!==undefined)return NMG_VOICE_LABELS[Number(el.dataset.voice)]+' output channel, 1–16. Applied after generation; notes and voice roles are unchanged. Inactive voices retain their assignment.';if(el.id==='nmgSync')return 'Follow DAW tempo, position, loops and transport for external MIDI. Turn off to use NMG tempo and transport.';if(el.id==='nmgOutput')return 'External MIDI plays your routed DAW instrument. Internal preview uses system audio while this window is open.';if(el.id==='nmgScale')return 'Scale the interface from 50% to 200%. Saved with the project and preset.';if(el.id==='save')return 'Save all current settings, the generated composition, channels and plugin options as a preset.';if(el.id==='load')return 'Restore an NMG preset, or import original HTML settings.';if(el.id==='play')return 'Start external MIDI when Sync to Host is off, or audition internal preview. With host sync, use your DAW transport.';")
script=core+app
# Replace floating help with a dedicated bottom panel, retaining original hint content.
script=script.replace("tip.hidden=true;document.body.append(tip);", "tip.hidden=false;tip.textContent='Hover or focus a control to see its explanation.';document.body.append(tip);")
script=script.replace("const hide=()=>{tip.hidden=true;};", "const hide=()=>{tip.hidden=false;tip.textContent='Hover or focus a control to see its explanation.';};")
script=script.replace("$('parameterTooltip').hidden=true;", "$('parameterTooltip').textContent='Hover or focus a control to see its explanation.';")
html=html.replace(re.search(r'<script>\s*(.*?)</script>',html,re.S).group(0),'<script>\n'+(root/'Web/theme.js').read_text(encoding='utf-8')+'\n'+bridge+'\n'+script+'\n</script>')
html=re.sub(r"@import url\('https://fonts.googleapis.com[^\n]+\n",'',html)
html=html.replace('<title>Numerical — MIDI Generator</title>','<title>NMG</title>').replace('numerical<span class="brand-dot">.</span>','NMG<span class="brand-dot">.</span>').replace('Your private composition studio','NMG · MIDI composition plugin')
html=html.replace('<span class="brand-sub">MIDI GENERATOR</span>', '<span class="brand-sub">MIDI GENERATOR</span><span id="nmgVersion" class="version-badge">v'+version+'</span>',1)
header_controls='''<div class="header-controls"><label>Playback destination<select id="nmgOutput"><option value="external">External MIDI → DAW instrument</option><option value="internal">Internal preview · system audio</option></select></label><label>Interface Scale<select id="nmgScale">'''+''.join(f'<option value="{v}"'+(' selected' if v==100 else '')+f'>{v}%</option>' for v in [50,75,90,100,125,150,175,200])+'''</select></label><label class="check-label"><input type="checkbox" id="nmgSync" checked> Sync to Host</label><button id="nmgTheme" aria-label="Change color theme"><svg viewBox="0 0 24 24"><path d="m4 11 7-7 8 8-7 7-8-8Z M8 2l6 6 M3 20h14 M20 14s-3 3-3 5a3 3 0 0 0 6 0c0-2-3-5-3-5Z"/></svg></button></div>'''
html=html.replace('</a><div class="header-right">','</a>'+header_controls+'<div class="header-right">',1)
controls='''<section class="panel native-panel"><div class="section-title"><h2>MIDI routing</h2><span id="nmgHostInfo" class="hint">Waiting for host transport</span></div><p id="nmgPreviewNote" class="hint"></p><details><summary>Voice → MIDI channel assignments <span>＋</span></summary><div id="nmgRoutes"></div><p class="hint">All voices initially use MIDI Channel 1. Shared-channel overlapping pitches are held until the last logical voice releases them. Original MIDI export and drag retain the source’s per-voice channels; these assignments control live host output.</p></details></section>'''
html=re.sub(r'<label class="volume-label".*?</label>','',html, count=1, flags=re.S)
html=html.replace('<button id="download"', '<button id="dragMidi" class="download" disabled>↗ Drag MIDI</button><button id="download"',1)
picker='''<dialog id="themeDialog" aria-labelledby="themeTitle"><button id="themeClose" class="close" aria-label="Close theme picker">×</button><h2 id="themeTitle">Color theme</h2><input id="themeHex" maxlength="7" aria-label="Theme hex color" spellcheck="false"><div class="theme-plane"><div id="themeSV" tabindex="0" role="group" aria-label="Saturation and brightness; use arrow keys"><span id="themeCursor"></span></div><input id="themeHue" type="range" min="0" max="360" value="90" aria-label="Hue"></div>'''+''.join(f'<label class="theme-rgb">{name}<output id="theme{c}Value"></output><input id="theme{c}" type="range" min="0" max="255" aria-label="{name} color value"></label>' for c,name in [('R','Red'),('G','Green'),('B','Blue')])+'''<button id="themeReset" class="secondary">Restore original green</button></dialog>'''
html=html.replace('<main>',picker+'<main>',1)
html=html.replace('<div class="lower-grid">',controls+'<div class="lower-grid">',1)
# Scrolling native content; fixed help never floats over controls.
css='''\nbody{padding-bottom:120px}#parameterTooltip{display:block!important;position:fixed!important;left:0!important;top:auto!important;bottom:0!important;width:100%!important;max-width:none!important;border-radius:0;min-height:62px;max-height:100px;overflow:auto;pointer-events:none;background:#26371e;color:#e4efd7;font-size:12px;z-index:1000}.native-panel{padding:20px}#nmgRoutes{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-top:18px}#nmgRoutes label{font-size:11px;color:#cfdfc0}#nmgRoutes select{margin-top:7px}.inactive-voice{opacity:.55}.source-panel,.settings-panel{min-width:0}header{height:64px}main{padding-top:20px}h1{font-size:28px}@media(max-width:760px){#nmgRoutes{grid-template-columns:1fr}.native-panel .fields{grid-template-columns:1fr}}\n'''
css+=(root/'Web/appearance.css').read_text(encoding='utf-8')
html=html.replace('</style>',css+'</style>',1)
# Theme only the original green-tinted CSS palette, preserving track colors and engine.
style=re.search(r'<style>(.*?)</style>',html,re.S).group(1)
palette=[]
def theme_color(match):
 value=match.group(0)[1:]
 if len(value)==3:value=''.join(c*2 for c in value)
 if len(value)!=6:return match.group(0)
 r,g,b=[int(value[i:i+2],16)/255 for i in (0,2,4)]
 h,l,s=colorsys.rgb_to_hls(r,g,b)
 if not(60<=h*360<=150 and s>.05):return match.group(0)
 key='--nmg-'+value
 if not any(p[0]==key for p in palette):palette.append((key,round(h*360,2),round(s*100,2),round(l*100,2)))
 return f'var({key},#{value})'
style=re.sub(r'#[0-9a-fA-F]{3,8}\b',theme_color,style)
html=re.sub(r'<style>.*?</style>',lambda _: '<style>'+style+'</style>',html,count=1,flags=re.S)
html=html.replace('<script>','<script>\nconst NMG_THEME_COLORS='+json.dumps(palette)+';\n',1)
(root/'Web/index.html').write_text(html,encoding='utf-8',newline='\n')
# Assert byte-for-byte mathematical modules are intact in the shipped UI.
for name in ['chords','engine']:
 original=(root/('Reference/'+name+'.js')).read_text(encoding='utf-8');assert original in html,name+' was altered'
(root/'Tests/packaged-core.js').write_text(core+'\nglobalThis.NMGReference={engine:module_engine,chords:module_chords};',encoding='utf-8',newline='\n')
print('Built Web/index.html; original engine and chords verified byte-for-byte.')
