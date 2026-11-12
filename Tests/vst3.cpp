#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <iostream>
void evaluateHostedEditor(juce::Component&,const juce::String&,std::function<void(juce::String,bool)>);
void snapshotEditor(juce::Component&,const juce::String&,std::function<void(bool)>);
class Check final:private juce::Timer {
 juce::AudioPluginFormatManager manager;
 std::unique_ptr<juce::AudioPluginInstance> plugin;
 std::unique_ptr<juce::AudioProcessorEditor> editor;
 int ticks=0,stage=0;bool busy=false;
 void open(){editor.reset(plugin->createEditorIfNeeded());if(!editor)throw std::runtime_error("No VST3 editor");editor->addToDesktop(juce::ComponentPeer::windowIsTemporary);editor->setVisible(true);}
 void finish(bool ok,juce::String message){result=ok?0:1;std::cout<<(ok?"PASS ":"FAIL ")<<message<<std::endl;stopTimer();juce::MessageManager::getInstance()->stopDispatchLoop();}
public:
 int result=1;
 Check(const juce::String& path){
  auto format=std::make_unique<juce::VST3PluginFormat>();juce::OwnedArray<juce::PluginDescription> types;format->findAllTypesForFile(types,path);
  if(types.size()!=1||types[0]->version!="1.1.1")throw std::runtime_error("VST3 factory version/count mismatch");
  manager.addFormat(std::move(format));juce::String error;plugin=manager.createPluginInstance(*types[0],48000,512,error);
  if(!plugin)throw std::runtime_error(error.toStdString());plugin->prepareToPlay(48000,512);open();startTimer(100);
 }
 ~Check(){editor.reset();if(plugin)plugin->releaseResources();}
 void timerCallback()override{
  if(++ticks>450){finish(false,"compiled VST3 test timed out at stage "+juce::String(stage));return;}
  if(busy)return;busy=true;
  if(stage==0){evaluateHostedEditor(*editor,R"JS((()=>{
    if(!globalThis.document.getElementById('nmgVersion')||!nmgBridge.current?.composition?.notes||!nmgBridge.dragPath)return 'waiting';
    if(document.getElementById('nmgVersion').textContent!=='v1.1.1')throw Error('header version');
    for(const id of ['nmgOutput','nmgScale','nmgSync','nmgTheme'])if(!document.getElementById(id).closest('header'))throw Error('missing header control');
    if(document.getElementById('volume'))throw Error('old volume UI');
    if(!document.getElementById('dragMidi'))throw Error('no drag MIDI');
    if(nmgBridge.current.settings.voices!==5||nmgBridge.plugin.channels.some(c=>c!==1))throw Error('voice defaults');
    const options=[...document.getElementById('transpose').options].map(x=>Number(x.value));
    if(options.some((x,i)=>i&&x>=options[i-1]))throw Error('octave ordering');
    document.getElementById('themeHex').value='FFC100';document.getElementById('themeHex').dispatchEvent(new Event('change'));
    document.getElementById('nmgSync').checked=false;document.getElementById('nmgSync').dispatchEvent(new Event('change'));
    document.getElementById('play').click();return 'ready';
  })())JS",[this](auto text,bool ok){busy=false;if(!ok&&ticks<40)return;if(!ok){finish(false,text);return;}if(text=="ready"){editor->setSize(1380,920);stage=1;ticks=0;}});
  }else if(stage==1){
   if(ticks<5){busy=false;return;}
   juce::AudioBuffer<float> audio(2,512);juce::MidiBuffer midi;int ons=0;
   for(int i=0;i<200;i++){plugin->processBlock(audio,midi);for(const auto m:midi){if(m.getMessage().isNoteOn())ons++;}}
   if(!ons){finish(false,"VST3 wrapper did not output MIDI");return;}
   editor.reset();open();if(editor->getWidth()!=1380||editor->getHeight()!=920){finish(false,"VST3 editor resize/reopen");return;}
   stage=2;ticks=0;busy=false;
  }else if(stage==2){
   if(ticks<10){busy=false;return;}
   evaluateHostedEditor(*editor,R"JS((()=>{
    if(document.getElementById('nmgVersion').textContent!=='v1.1.1')throw Error('reopen version');
    if(nmgBridge.plugin.theme!=='FFC100')throw Error('reopen theme');
    return 'ready';
   })())JS",[this](auto text,bool ok){busy=false;if(!ok){finish(false,text);return;}stage=3;busy=true;
    snapshotEditor(*editor,juce::File::getCurrentWorkingDirectory().getChildFile("build-mac/vst3-1.1.1.png").getFullPathName(),[this](bool captured){finish(captured,"loaded actual NMG.vst3 1.1.1: header/version, new controls, five voices/channel 1, theme, descending octaves, removed volume, MIDI file preparation, native MIDI output, resized editor reopen");});
   });
  }
 }
};
int main(int argc,char** argv){try{juce::ScopedJuceInitialiser_GUI gui;if(argc!=2)return 2;Check check(juce::File(argv[1]).getFullPathName());juce::MessageManager::getInstance()->runDispatchLoop();return check.result;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}}
