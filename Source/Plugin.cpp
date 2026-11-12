#include "Plugin.h"
#include "BinaryData.h"
#if JUCE_MAC
#include "MidiDrag.h"
#endif
using namespace juce;
NMGProcessor::NMGProcessor():AudioProcessor(BusesProperties().withOutput("Output",AudioChannelSet::stereo(),true)){state=var(new DynamicObject());}
void NMGProcessor::prepareToPlay(double rate,int){sampleRate=rate;output.ensureSize(4*1024*1024);scheduler=nmg::Scheduler();}
bool NMGProcessor::isBusesLayoutSupported(const BusesLayout& b)const{return b.getMainOutputChannelSet()==AudioChannelSet::stereo();}
void NMGProcessor::processBlock(AudioBuffer<float>& audio,MidiBuffer& midi){ScopedNoDenormals no;audio.clear();output.clear();nmg::Score* s;do{s=score.load();hazard.store(s);}while(s!=score.load());
 double bpm=s?s->bpm:120,beat=freeBeat;bool active=play.load();auto seek=seekVersion.load();bool reset=seek!=handledSeek;if(reset){freeBeat=seekBeat.load();beat=freeBeat;handledSeek=seek;}auto mode=modeVersion.load();reset=reset||mode!=handledMode;handledMode=mode;
 if(sync.load()){active=false;if(auto* ph=getPlayHead())if(auto pos=ph->getPosition()){active=pos->getIsPlaying();if(auto t=pos->getBpm())bpm=*t;if(auto ppq=pos->getPpqPosition())beat=*ppq;else if(auto seconds=pos->getTimeInSeconds())beat=*seconds*bpm/60;else active=false;}}
 if(!std::isfinite(bpm)||bpm<=0)bpm=120;hostBpm=bpm;hostRunning=active;const double bps=bpm/(60*sampleRate);
 scheduler.process(s,beat,bps,audio.getNumSamples(),active&&external.load(),reset,[this](int ch,int pitch,int vel,bool on,int offset){if(on)output.addEvent(MidiMessage::noteOn(ch,pitch,(uint8)vel),offset);else output.addEvent(MidiMessage::noteOff(ch,pitch),offset);});
 if(!sync.load()&&active){freeBeat+=audio.getNumSamples()*bps;if(s&&freeBeat>=s->end)play=false;}displayedBeat=beat;hazard.store(nullptr);midi.swapWith(output);
}
void NMGProcessor::rememberEditorSize(int width,int height){
 if(width<640||height<480)return;
 {const ScopedLock lock(stateLock);DynamicObject::Ptr size=new DynamicObject();size->setProperty("width",width);size->setProperty("height",height);state.getDynamicObject()->setProperty("editorSize",var(size.get()));}
 updateHostDisplay(ChangeDetails().withNonParameterStateChanged(true));
}
juce::Point<int> NMGProcessor::editorSize(){const ScopedLock lock(stateLock);auto size=state["editorSize"];return {size.hasProperty("width")?jlimit(640,2400,(int)size["width"]):1200,size.hasProperty("height")?jlimit(480,1800,(int)size["height"]):850};}
var NMGProcessor::getState(){const ScopedLock lock(stateLock);return JSON::parse(JSON::toString(state));}
void NMGProcessor::acceptState(const var& incoming,bool preserveEditorSize){
 if(!incoming.isObject())return;
 bool playbackChanged;
 {const ScopedLock lock(stateLock);
  playbackChanged=JSON::toString(state["composition"])!=JSON::toString(incoming["composition"])
   ||JSON::toString(state["plugin"]["channels"])!=JSON::toString(incoming["plugin"]["channels"])
   ||state["plugin"]["sync"]!=incoming["plugin"]["sync"]
   ||state["plugin"]["external"]!=incoming["plugin"]["external"];
  auto rememberedSize=state["editorSize"];state=JSON::parse(JSON::toString(incoming));if((preserveEditorSize||!state.hasProperty("editorSize"))&&rememberedSize.isObject())state.getDynamicObject()->setProperty("editorSize",rememberedSize);
 }
 if(playbackChanged)publish();
 stateRevision++;
}
void NMGProcessor::publish(){const ScopedLock lock(stateLock);const auto composition=state["composition"],plugin=state["plugin"];auto s=std::make_unique<nmg::Score>();s->bpm=(double)composition["settings"]["bpm"];if(!std::isfinite(s->bpm)||s->bpm<20)s->bpm=120;s->end=(double)composition["maxBeat"];if(auto* channels=plugin["channels"].getArray())for(int i=0;i<jmin(8,channels->size());i++)s->channels[(size_t)i]=jlimit(1,16,(int)(*channels)[i]);
 if(auto* ns=composition["notes"].getArray())for(const auto& n:*ns){nmg::Note note{(double)n["startBeat"],(double)n["durationBeats"],(int)n["midiNote"],(int)n["velocity"],(int)n["track"]};if(std::isfinite(note.start)&&std::isfinite(note.duration)&&note.start>=0&&note.duration>0&&isPositiveAndBelow(note.voice,8)&&isPositiveAndBelow(note.pitch,128)&&note.velocity>0&&note.velocity<=127)s->notes.push_back(note);}
 s->prepare();s->revision=stateRevision.load()+1;sync=plugin.hasProperty("sync")?(bool)plugin["sync"]:true;external=plugin.hasProperty("external")?(bool)plugin["external"]:false;
 auto old=std::move(owned);owned=std::move(s);score.store(owned.get());if(old)retired.push_back(std::move(old));auto* protectedScore=hazard.load();retired.erase(std::remove_if(retired.begin(),retired.end(),[&](auto& p){return p.get()!=protectedScore;}),retired.end());modeVersion++;
}
void NMGProcessor::command(const var& v){const String type=v["type"].toString();if(type=="play")play=true;else if(type=="pause")play=false;else if(type=="stop"){play=false;seekBeat=0;seekVersion++;}else if(type=="seek"){seekBeat=jmax(0.0,(double)v["beat"]);seekVersion++;} }
void NMGProcessor::getStateInformation(MemoryBlock& dest){const ScopedLock lock(stateLock);auto json=JSON::toString(state);dest.replaceAll(json.toRawUTF8(),json.getNumBytesAsUTF8());}
void NMGProcessor::setStateInformation(const void* data,int size){if(size<=0||size>64*1024*1024)return;auto parsed=JSON::parse(String::fromUTF8((const char*)data,size));if(parsed.isObject())acceptState(parsed);}
class NMGEditor final:public AudioProcessorEditor,private Timer {
 class Browser final:public WebBrowserComponent {public:using WebBrowserComponent::WebBrowserComponent;bool pageAboutToLoad(const String& url)override{return url.startsWith(WebBrowserComponent::getResourceProviderRoot())||url=="about:blank";}};
 NMGProcessor& proc;std::unique_ptr<Browser> browser;std::unique_ptr<FileChooser> chooser;uint64_t seen=0;bool ready=false,dragInProgress=false;
 File dragFolder=File::getSpecialLocation(File::tempDirectory).getChildFile("NMG-MIDI-"+Uuid().toString()),dragFile;
 #if JUCE_MAC
 NativeMidiDrag midiDrag;
 #endif
 static std::optional<WebBrowserComponent::Resource> resource(const String& path){if(path!="/"&&path!="/index.html"&&path!="/nmg-" JucePlugin_VersionString ".html")return {};std::vector<std::byte> data((size_t)BinaryData::index_htmlSize);std::memcpy(data.data(),BinaryData::index_html,(size_t)BinaryData::index_htmlSize);return WebBrowserComponent::Resource{std::move(data),"text/html"};}
 void sendState(){auto size=proc.editorSize();if(getWidth()!=size.x||getHeight()!=size.y)setSize(size.x,size.y);browser->emitEventIfBrowserIsVisible("nmgState",proc.getState());seen=proc.stateRevision;}
 void message(const var& msg){const String type=msg["type"].toString();if(type=="ready"){ready=true;sendState();}else if(type=="state"){proc.acceptState(msg["state"],true);seen=proc.stateRevision;}else if(type=="prepareDrag"){
 MemoryOutputStream bytes;const auto encoded=msg["base64"].toString();
 if(encoded.length()<32*1024*1024&&Base64::convertFromBase64(bytes,encoded)&&bytes.getDataSize()>14&&std::memcmp(bytes.getData(),"MThd",4)==0){
 dragFolder.createDirectory();auto name=File::createLegalFileName(msg["name"].toString());if(!name.endsWithIgnoreCase(".mid"))name+=".mid";
 auto candidate=dragFolder.getNonexistentChildFile(name.upToLastOccurrenceOf(".",false,false),".mid");
 if(candidate.replaceWithData(bytes.getData(),bytes.getDataSize())){dragFile=candidate;
 #if JUCE_MAC
 midiDrag.setFile(dragFile);
 #endif
 browser->emitEventIfBrowserIsVisible("nmgDragReady",dragFile.getFullPathName());}
 }
 }else if(type=="dragBounds"){
 #if JUCE_MAC
 const Rectangle<int> rect((int)msg["x"],(int)msg["y"],(int)msg["width"],(int)msg["height"]);
 midiDrag.place(*this,rect,(bool)msg["visible"]&&dragFile.existsAsFile());
 #endif
 }else if(type=="dragMidi"){
 #if !JUCE_MAC
 if(dragFile.existsAsFile()&&!dragInProgress){
 dragInProgress=true;Component::SafePointer<NMGEditor> safe(this);
 if(!DragAndDropContainer::performExternalDragDropOfFiles({dragFile.getFullPathName()},false,browser.get(),[safe]{MessageManager::callAsync([safe]{if(safe)safe->dragInProgress=false;});}))dragInProgress=false;
 }
 #endif
 }else if(type=="saveFile"){
 const auto data=msg["base64"].toString();const auto filename=File::createLegalFileName(msg["name"].toString());chooser=std::make_unique<FileChooser>("Save NMG file",File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(filename),"*");Component::SafePointer<NMGEditor> safe(this);chooser->launchAsync(FileBrowserComponent::saveMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::warnAboutOverwriting,[safe,data](const FileChooser& fc){if(!safe)return;auto file=fc.getResult();if(file==File{})return;MemoryOutputStream bytes;if(Base64::convertFromBase64(bytes,data)&&file.replaceWithData(bytes.getData(),bytes.getDataSize()))safe->browser->emitEventIfBrowserIsVisible("nmgNotice","File saved.");else safe->browser->emitEventIfBrowserIsVisible("nmgNotice","Could not save file.");});
 }else if(type=="loadPreset"){chooser=std::make_unique<FileChooser>("Load NMG preset",File{},"*.json");Component::SafePointer<NMGEditor> safe(this);chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles,[safe](const FileChooser& fc){if(!safe)return;auto f=fc.getResult();if(f.existsAsFile()&&f.getSize()<64*1024*1024)safe->browser->emitEventIfBrowserIsVisible("nmgPreset",f.loadFileAsString());});}else proc.command(msg);}
 void timerCallback()override{if(!ready)return;if(seen!=proc.stateRevision)sendState();DynamicObject::Ptr obj=new DynamicObject();obj->setProperty("beat",proc.displayedBeat.load());obj->setProperty("bpm",proc.hostBpm.load());obj->setProperty("playing",proc.hostRunning.load());obj->setProperty("sync",proc.sync.load());browser->emitEventIfBrowserIsVisible("nmgTransport",var(obj.get()));}
public:NMGEditor(NMGProcessor& p):AudioProcessorEditor(p),proc(p){const auto rememberedSize=p.editorSize();auto opts=WebBrowserComponent::Options{}.withNativeIntegrationEnabled().withResourceProvider(resource).withEventListener("nmg",[this](const var& v){message(v);});
 #if JUCE_WINDOWS
 const auto webData=File::getSpecialLocation(File::windowsLocalAppData).getChildFile("NMG/WebView2");
 webData.createDirectory();
 opts=opts.withBackend(WebBrowserComponent::Options::Backend::webview2)
 .withWinWebView2Options(WebBrowserComponent::Options::WinWebView2{}.withUserDataFolder(webData));
 #endif
 browser=std::make_unique<Browser>(opts);addAndMakeVisible(*browser);setResizable(true,true);setResizeLimits(640,480,2400,1800);setSize(rememberedSize.x,rememberedSize.y);browser->goToURL(WebBrowserComponent::getResourceProviderRoot()+"nmg-" JucePlugin_VersionString ".html");startTimerHz(15);}
 ~NMGEditor()override{stopTimer();}
 void resized()override{if(browser)browser->setBounds(getLocalBounds());proc.rememberEditorSize(getWidth(),getHeight());}
};
AudioProcessorEditor* NMGProcessor::createEditor(){return new NMGEditor(*this);}AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new NMGProcessor();}
