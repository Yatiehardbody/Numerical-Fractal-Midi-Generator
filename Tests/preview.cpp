#include "Plugin.h"
#include <iostream>
class PreviewCheck final:private juce::Timer {
 NMGProcessor processor;std::unique_ptr<juce::AudioProcessorEditor> editor;
 juce::WebBrowserComponent* browser=nullptr;int ticks=0;bool started=false,busy=false;
 void finish(bool ok,const juce::String& text){result=ok?0:1;std::cout<<(ok?"PASS ":"FAIL ")<<text<<std::endl;stopTimer();juce::MessageManager::getInstance()->stopDispatchLoop();}
public:
 int result=1;
 PreviewCheck(){editor.reset(processor.createEditor());editor->addToDesktop(juce::ComponentPeer::windowIsTemporary);editor->setVisible(true);browser=dynamic_cast<juce::WebBrowserComponent*>(editor->getChildComponent(0));startTimer(200);}
 void timerCallback()override{
  if(++ticks>600){finish(false,"Preview render timed out");return;}if(busy)return;
  if(!started){if(!processor.getState()["composition"]["notes"].isArray())return;started=true;browser->evaluateJavascript(juce::File::getCurrentWorkingDirectory().getChildFile("Tests/audio-render.js").loadFileAsString());return;}
  busy=true;browser->evaluateJavascript("JSON.stringify(globalThis.nmgAudioCheck||{})",[this](auto evaluation){busy=false;if(auto* e=evaluation.getError()){finish(false,e->message);return;}auto text=evaluation.getResult()->toString();auto state=juce::JSON::parse(text);if(state["status"]==juce::var("passed")){juce::File::getCurrentWorkingDirectory().getChildFile("preview-render-results.json").replaceWithText(text);finish(true,"Embedded browser audio renders: continuous note envelopes, pause fades, dense mix headroom and sample playback. "+text);}else if(state["status"]==juce::var("failed"))finish(false,text);});
 }
};
int main(){juce::ScopedJuceInitialiser_GUI init;PreviewCheck check;juce::MessageManager::getInstance()->runDispatchLoop();return check.result;}
