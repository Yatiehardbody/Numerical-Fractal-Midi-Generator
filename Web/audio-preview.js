// Preview-only audio fixes. Generated notes, velocities and MIDI bytes are untouched.
(() => {
  const proto = module_audio.AudioPreview.prototype;
  const originalSchedule = proto.schedule;
  function envelope(param,start,end,level,harmony=false) {
    const duration=end-start;
    const attack=Math.min(harmony?.07:.015,duration*.2);
    const decay=Math.min(.045,duration*.15);
    const release=Math.min(harmony?.15:.06,duration*.3);
    param.setValueAtTime(0,start);
    param.linearRampToValueAtTime(level,start+attack);
    param.linearRampToValueAtTime(level*.72,start+attack+decay);
    param.setValueAtTime(level*.72,end-release);
    param.linearRampToValueAtTime(0,end);
    return [[start,0],[start+attack,level],[start+attack+decay,level*.72],[end-release,level*.72],[end,0]];
  }
  function levelAt(points,time){
    if(time<=points[0][0])return points[0][1];
    for(let i=1;i<points.length;i++)if(time<=points[i][0]){
      const [a,x]=points[i-1],[b,y]=points[i];return b===a?y:x+(y-x)*(time-a)/(b-a);
    }
    return points[points.length-1][1];
  }
  // Retain a reference only for offline before/after verification.
  module_audio.originalSchedule = originalSchedule;
  proto.schedule = function(n,start,end) {
    if(end<=start)return;
    if(this.instrument!=='default'){this.scheduleSample(n,start,end);return;}
    const ctx=this.context,osc=ctx.createOscillator(),gain=ctx.createGain(),filter=ctx.createBiquadFilter();
    osc.type=['triangle','sine','sine','triangle','sine','sine','triangle','sine'][n.track];
    osc.frequency.value=440*2**((n.midiNote-69)/12);
    filter.type='lowpass';filter.frequency.value=n.track===1?850:n.track===2?1800:4000;
    const points=envelope(gain.gain,start,end,n.velocity/127*(n.track===2?.32:.6),n.track===2);
    osc.connect(filter);filter.connect(gain);gain.connect(this.master);
    osc.start(start);osc.stop(end+.005);
    this.nodes.add(osc);
    this.previewVoices ||= new Map();this.previewVoices.set(osc,{gain,start,end,points});
    osc.onended=()=>{osc.disconnect();filter.disconnect();gain.disconnect();this.nodes.delete(osc);this.previewVoices.delete(osc);};
  };
  // Track the per-note output gain for samples too, so seeking/instrument changes
  // fade active sources instead of cutting them at an arbitrary waveform phase.
  proto.scheduleSample=function(n,start,end){
    if(end<=start)return;
    const bank=this.banks.get(this.instrument);if(!bank?.size)return;
    const key=[...bank.keys()].reduce((best,k)=>Math.abs(k-n.midiNote)<Math.abs(best-n.midiNote)?k:best);
    const ctx=this.context,source=ctx.createBufferSource(),gain=ctx.createGain();
    source.buffer=bank.get(key);source.playbackRate.value=2**((n.midiNote-key)/12);
    const duration=end-start,attack=Math.min(.004,duration/3),
      release=Math.min(this.instrument==='pad'?.35:this.instrument==='harp'?.18:.08,duration/3),
      level=n.velocity/127*(n.track===2?.45:.8)*(this.sampleGains.get(source.buffer)||1);
    gain.gain.setValueAtTime(0,start);
    gain.gain.linearRampToValueAtTime(level,start+attack);
    gain.gain.setValueAtTime(level,end-release);
    gain.gain.linearRampToValueAtTime(0,end);
    source.connect(gain);gain.connect(this.master);source.start(start);source.stop(end+.005);
    this.nodes.add(source);
    this.previewVoices ||= new Map();
    this.previewVoices.set(source,{gain,start,end,points:[[start,0],[start+attack,level],[end-release,level],[end,0]]});
    source.onended=()=>{source.disconnect();gain.disconnect();this.nodes.delete(source);this.previewVoices.delete(source);};
  };
  proto.pause=function(){
    for(const node of this.clickNodes){try{node.stop();}catch{}}this.clickNodes.clear();
    this.playToken++;
    if(this.playing)this.offset=Math.min(this.composition.settings.seconds,this.context.currentTime-this.startedAt);
    this.playing=false;clearInterval(this.timer);
    const now=this.context?.currentTime||0;
    for(const node of this.nodes){
      const voice=this.previewVoices?.get(node);
      try{
        if(voice&&voice.start<=now){
          // Compute the actual linear envelope level. This also avoids WebKit's
          // cancelAndHold behavior resuming from an earlier scheduled value.
          const held=levelAt(voice.points,now);
          voice.gain.gain.cancelScheduledValues(now);
          voice.gain.gain.setValueAtTime(held,now);
          voice.gain.gain.linearRampToValueAtTime(0,now+.015);
          voice.points=[[now,held],[now+.015,0]];
          node.stop(now+.018);
        }else node.stop(); // A source scheduled in the future has not sounded.
      }catch{}
    }
    // Keep active sources until onended disconnects them, including during seek.
    this.onState(false);
  };
})();
