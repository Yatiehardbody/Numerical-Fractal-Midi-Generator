globalThis.nmgAudioCheck={status:'running'};
(async()=>{
  const Audio=module_audio.AudioPreview, Offline=window.OfflineAudioContext||window.webkitOfflineAudioContext;
  const rate=48000;
  function create(seconds,mastering=false){
    const a=new Audio(()=>{},()=>{});a.context=new Offline(1,Math.ceil(seconds*rate),rate);
    a.master=a.context.createGain();a.master.gain.value=mastering?.25:1;
    if(mastering){const comp=a.context.createDynamicsCompressor();comp.threshold.value=-18;comp.ratio.value=5;a.master.connect(comp);comp.connect(a.context.destination);}
    else a.master.connect(a.context.destination);
    return a;
  }
  function stats(data){let peak=0,maxCurvature=0,energy=0;
    for(let i=2;i<data.length;i++){if(!Number.isFinite(data[i]))throw Error('Invalid audio sample');peak=Math.max(peak,Math.abs(data[i]));energy+=data[i]**2;maxCurvature=Math.max(maxCurvature,Math.abs(data[i]-2*data[i-1]+data[i-2]));}
    return {peak,maxCurvature,rms:Math.sqrt(energy/data.length)};
  }
  const comparisons=[];
  for(const duration of [.1,.31,.91]){
    const results=[];
    for(const old of [true,false]){
      const a=create(duration+.1),note={track:1,midiNote:36,velocity:110};
      (old?module_audio.originalSchedule:Audio.prototype.schedule).call(a,note,.02,.02+duration);
      results.push(stats((await a.context.startRendering()).getChannelData(0)));
    }
    if(results[1].maxCurvature>=results[0].maxCurvature*.25)throw Error('Synth discontinuity regression '+JSON.stringify(results));
    comparisons.push({duration,before:results[0],after:results[1]});
  }
  // Stop mid-wave in WebKit's actual offline renderer; no abrupt node cutoff.
  const paused=create(1);paused.composition={settings:{seconds:1}};paused.playing=true;paused.startedAt=0;
  paused.schedule({track:1,midiNote:36,velocity:110},.02,.8);
  const suspension=paused.context.suspend(.137).then(()=>{paused.pause();return paused.context.resume();});
  const pauseResult=stats((await paused.context.startRendering()).getChannelData(0));await suspension;
  if(pauseResult.maxCurvature>.004)throw Error('Hard pause/seek cutoff detected '+JSON.stringify(pauseResult));
  // Stress the complete existing preview mix with eight-voice dense material.
  const dense=module_engine.compose({...module_engine.defaults(),engine:'Density',voices:8,seconds:8,complexity:'Very High',seed:'preview-density'});
  const mix=create(8.2,true),bps=dense.settings.bpm/60;
  for(const note of dense.notes)mix.schedule(note,.02+note.startBeat/bps,.02+(note.startBeat+note.durationBeats)/bps);
  const mixResult=stats((await mix.context.startRendering()).getChannelData(0));
  if(mixResult.peak>=1)throw Error('Preview clipping '+JSON.stringify(mixResult));
  // Actual sample scheduling and gain cleanup, using a synthetic sample bank.
  const sampled=create(.5);sampled.instrument='piano';const buffer=sampled.context.createBuffer(1,rate,rate),data=buffer.getChannelData(0);
  for(let i=0;i<data.length;i++)data[i]=Math.sin(2*Math.PI*110*i/rate)*.4;
  sampled.banks.set('piano',new Map([[45,buffer]]));sampled.schedule({track:1,midiNote:45,velocity:100},.02,.4);
  const sampleResult=stats((await sampled.context.startRendering()).getChannelData(0));
  if(sampleResult.peak===0||sampleResult.maxCurvature>.004)throw Error('Sample fade regression');
  globalThis.nmgAudioCheck={status:'passed',comparisons,pauseResult,mixResult,sampleResult,notes:dense.notes.length};
})().catch(e=>{globalThis.nmgAudioCheck={status:'failed',error:String(e),stack:e.stack};});
