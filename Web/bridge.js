// Host/UI adapter. Original mathematical modules are never altered.
const NMG_VOICE_LABELS=['Voice 1 (Lead)','Voice 2 (Bass)','Voice 3 (Harmony)','Voice 4 (Counterpoint)','Voice 5 (Additional Melody)','Voice 6 (Mathematical Texture)','Voice 7 (Canon Voice)','Voice 8 (Upper Texture)'];
const nmgBridge={ready:false,restoring:false,plugin:{channels:[1,1,1,1,1,1,1,1],sync:true,external:true,scale:100},current:null,
 send(type,extra={}){window.__JUCE__?.backend.emitEvent('nmg',{type,...extra});},
 publish(c,settings){const changed=this.current?.composition!==c;this.current={composition:c,settings:structuredClone(settings),plugin:structuredClone(this.plugin)};if(this.ready&&!this.restoring)this.send('state',{state:this.current});if(changed&&this.hooks)this.prepareDrag();},
 saveConfig(){if(this.current){this.current.plugin=structuredClone(this.plugin);if(this.ready&&!this.restoring)this.send('state',{state:this.current});}},
 file(data,name,type){const reader=new FileReader();reader.onload=()=>this.send('saveFile',{name,base64:reader.result.split(',')[1]});reader.readAsDataURL(new Blob([data],{type}));},
 prepareDrag(){if(!this.current||!this.hooks)return;document.getElementById('dragMidi').disabled=true;const version=this.dragVersion=(this.dragVersion||0)+1;const reader=new FileReader();reader.onload=()=>{if(this.dragVersion===version)this.send('prepareDrag',{name:this.hooks.midiName(),base64:reader.result.split(',')[1]});};reader.readAsDataURL(new Blob([this.hooks.midi()],{type:'audio/midi'}));},
 dragBounds(){const b=document.getElementById('dragMidi'),r=b.getBoundingClientRect();this.send('dragBounds',{x:Math.round(r.left),y:Math.round(r.top),width:Math.round(r.width),height:Math.round(r.height),visible:!b.disabled&&!document.querySelector('dialog[open]')&&r.top>=0&&r.bottom<innerHeight-100&&r.left>=0&&r.right<=innerWidth});},
 setup(hooks){const byId=id=>document.getElementById(id);this.hooks=hooks;
 nmgTheme.setup();
 const routing=byId('nmgRoutes');routing.innerHTML=NMG_VOICE_LABELS.map((label,i)=>`<label>${label}<select data-voice="${i}" aria-label="${label} MIDI channel">${Array.from({length:16},(_,ch)=>`<option value="${ch+1}">MIDI Channel ${ch+1}</option>`).join('')}</select></label>`).join('');
 routing.onchange=e=>{const index=Number(e.target.dataset.voice);this.plugin.channels[index]=Number(e.target.value);this.saveConfig();};
 byId('nmgSync').onchange=e=>{this.plugin.sync=e.target.checked;hooks.stopPreview();this.send('stop');this.saveConfig();};
 byId('nmgOutput').onchange=e=>{this.plugin.external=e.target.value==='external';hooks.stopPreview();this.send('stop');this.saveConfig();this.refresh();};
 byId('nmgScale').onchange=e=>{this.plugin.scale=Number(e.target.value);this.refresh();this.saveConfig();};
 byId('save').textContent='↓ Save Preset';byId('load').textContent='↥ Load Preset';byId('download').textContent='↓ Original MIDI';
 byId('save').onclick=()=>{if(!this.current)return;this.file(JSON.stringify({format:'nmg-plugin',version:1,...this.current,plugin:this.plugin},null,2),hooks.presetName(),'application/json');};
 byId('load').onclick=()=>this.send('loadPreset');
 hooks.previewSeek=byId('roll').onclick;
 const originalPlay=byId('play').onclick,originalPause=byId('pause').onclick,originalStop=byId('stop').onclick,originalSeek=byId('timeline').oninput;
 byId('play').onclick=()=>{if(this.plugin.external){this.send('play');if(this.plugin.sync)hooks.notice('Sync to Host is on. Start playback in your DAW.');}else originalPlay();};
 byId('pause').onclick=()=>{this.send('pause');originalPause();};byId('stop').onclick=()=>{this.send('stop');originalStop();};
 byId('timeline').oninput=e=>{if(this.plugin.external){if(this.plugin.sync){hooks.notice('Seek with the DAW transport when Sync to Host is on.');return;}this.send('seek',{beat:Number(e.target.value)*this.current.composition.settings.bpm/60});}else originalSeek(e);};
 byId('roll').onclick=e=>{if(this.plugin.external){const r=byId('roll').getBoundingClientRect();if(!this.plugin.sync)this.send('seek',{beat:Math.max(0,Math.min(1,(e.clientX-r.left-38)/(r.width-38)))*this.current.composition.maxBeat});}else hooks.previewSeek(e);};
 document.addEventListener('keydown',e=>{if(e.code==='Space'&&!['INPUT','SELECT','TEXTAREA','BUTTON'].includes(document.activeElement.tagName)){e.preventDefault();e.stopImmediatePropagation();byId('play').click();}},true);
 const backend=window.__JUCE__?.backend;
 backend?.addEventListener('nmgState',state=>{this.ready=true;if(state?.composition?.notes){this.restoring=true;this.plugin={channels:[1,1,1,1,1,1,1,1],sync:true,external:true,scale:100,...state.plugin};this.plugin.channels=Array.from({length:8},(_,i)=>Math.min(16,Math.max(1,Number(this.plugin.channels?.[i])||1)));hooks.restore(state);this.current=state;this.restoring=false;this.refresh();this.prepareDrag();}else{this.plugin={channels:[1,1,1,1,1,1,1,1],sync:true,external:true,scale:100};this.refresh();hooks.initial();}});
 backend?.addEventListener('nmgPreset',text=>{try{const data=JSON.parse(text);if(data.format==='nmg-plugin'&&data.composition?.notes){this.restoring=true;this.plugin={channels:[1,1,1,1,1,1,1,1],sync:true,external:true,scale:100,...data.plugin};hooks.restore(data);this.restoring=false;this.refresh();this.publish(data.composition,data.settings);}else if(data.format==='numerical-settings'){hooks.legacy(data.settings);this.refresh();}else throw Error('Unrecognized preset');hooks.notice('Preset loaded.');}catch(e){this.restoring=false;hooks.notice(e.message);}});
 backend?.addEventListener('nmgNotice',s=>hooks.notice(s));
 backend?.addEventListener('nmgTransport',s=>{byId('nmgHostInfo').textContent=`Host ${s.playing?'playing':'stopped'} · ${Number(s.bpm).toFixed(1)} BPM · beat ${Math.max(0,Number(s.beat)).toFixed(1)}`;if(this.plugin.external){hooks.position(s.beat);byId('play').disabled=s.playing&&!this.plugin.sync;byId('pause').disabled=!s.playing;}});
 backend?.addEventListener('nmgDragReady',path=>{this.dragPath=path;byId('dragMidi').disabled=false;this.dragBounds();});
 byId('dragMidi').onpointerdown=()=>{this.dragRequested=false;};
 byId('dragMidi').onpointermove=e=>{if(e.buttons===1&&!this.dragRequested&&!byId('dragMidi').disabled){this.dragRequested=true;this.send('dragMidi');}};
 addEventListener('scroll',()=>this.dragBounds(),true);addEventListener('resize',()=>this.dragBounds());new ResizeObserver(()=>this.dragBounds()).observe(document.querySelector('main'));document.querySelectorAll('dialog').forEach(d=>new MutationObserver(()=>this.dragBounds()).observe(d,{attributes:true,attributeFilter:['open']}));
 this.refresh();this.send('ready');
 },
 refresh(){const $=id=>document.getElementById(id);nmgTheme.apply('#'+(this.plugin.theme||'c3ee83'));requestAnimationFrame(()=>this.dragBounds());$('nmgSync').checked=!!this.plugin.sync;$('nmgOutput').value=this.plugin.external?'external':'internal';$('nmgScale').value=this.plugin.scale;document.querySelector('main').style.zoom=this.plugin.scale/100;document.querySelectorAll('#nmgRoutes select').forEach((el,i)=>el.value=this.plugin.channels[i]);$('nmgPreviewNote').textContent=this.plugin.external?'External MIDI: route NMG’s output port to your instrument. No program changes are sent.':'Internal preview auditions through system audio while the editor is open. Host sync applies to external MIDI only.';},
 voiceActivity(count){document.querySelectorAll('#nmgRoutes label').forEach((label,i)=>{label.classList.toggle('inactive-voice',i>=count);label.setAttribute('aria-description',i>=count?'Inactive until voice count is increased. Channel assignment is retained.':'Active voice');});}
};
