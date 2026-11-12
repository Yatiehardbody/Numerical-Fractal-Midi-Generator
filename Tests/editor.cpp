#include "Plugin.h"
#include <iostream>
#if JUCE_MAC
void snapshotEditor(juce::Component&,const juce::String&,std::function<void(bool)>);
#else
void snapshotEditor(juce::Component&,const juce::String&,std::function<void(bool)> f){f(true);}
#endif

// Automated embedded browser smoke test. This never launches a DAW.
class EditorCheck final : private juce::Timer {
    NMGProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::WebBrowserComponent* browser = nullptr;
    int ticks = 0, stage = 0;
 bool reopened=false;
    juce::String originalNotes;
public:
    int result = 1;
    EditorCheck() {
        editor.reset(processor.createEditor());
        editor->addToDesktop(juce::ComponentPeer::windowIsTemporary);
        editor->setVisible(true);
        browser = dynamic_cast<juce::WebBrowserComponent*>(editor->getChildComponent(0));
        startTimer(100);
    }
    void finish(bool ok, const juce::String& message) {
        result = ok ? 0 : 1;
        std::cout << (ok ? "PASS " : "FAIL ") << message << std::endl;
        stopTimer();
        juce::MessageManager::getInstance()->stopDispatchLoop();
    }
    void timerCallback() override {
        if (++ticks > 300) { finish(false, "embedded editor timed out at stage " + juce::String(stage)); return; }
        auto state = processor.getState();
        if (stage == 0 && state["composition"]["notes"].isArray()) {
            auto* channels = state["plugin"]["channels"].getArray();
            if ((int)state["settings"]["voices"] != 5 || !channels || channels->size() != 8) { finish(false, "initial defaults"); return; }
            for (auto channel : *channels) if ((int)channel != 1) { finish(false, "initial channel"); return; }
            if(state["settings"]["preset"].toString()!="Fractal"||state["settings"]["engine"].toString()!="AUTO"||state["plugin"]["theme"].toString()!="3EE23E"||(bool)state["plugin"]["external"]){finish(false,"Fractal/AUTO, green/internal preview defaults");return;}
            originalNotes = juce::JSON::toString(state["composition"]["notes"]);
            stage = 1;
            browser->evaluateJavascript(R"JS((()=>{
              if(document.getElementById('nmgOutput').value!=='internal'||document.getElementById('themeHex').value!=='3EE23E')throw Error('initial UI defaults');
              if(![...document.getElementById('scale').options].some(o=>o.value==='Chromatic'))throw Error('Chromatic option');
              const roll=document.getElementById('roll'),pixel=()=>Array.from(roll.getContext('2d').getImageData(roll.width-1,10,1,1).data);
              const green=pixel();if(green[1]<=green[0]||green[1]<=green[2])throw Error('piano roll default color');
              document.getElementById('themeHex').value='0066FF';document.getElementById('themeHex').dispatchEvent(new Event('change'));
              const blue=pixel();if(blue[2]<=blue[0]||blue[2]<=blue[1])throw Error('piano roll theme update');
              document.getElementById('themeReset').click();
              const routes=[...document.querySelectorAll('#nmgRoutes select')];
              if(routes.length!==8 || routes.some(x=>x.options.length!==16))throw Error('routing UI');
              if(!document.getElementById('legend').textContent.includes('Voice 2 (Bass)'))throw Error('voice labels');
              routes[1].value='16';routes[1].dispatchEvent(new Event('change',{bubbles:true}));
              return true;
            })())JS", [this](auto evaluation){if(auto* e=evaluation.getError())finish(false,e->message);});
        } else if (stage == 1 && (int)state["plugin"]["channels"][1] == 16) {
            if (originalNotes != juce::JSON::toString(state["composition"]["notes"])) { finish(false, "routing altered notes"); return; }
            stage = 2;
            browser->evaluateJavascript(R"JS((()=>{
              document.getElementById('voices').value='8';
              document.getElementById('regenerate').click();
              return true;
            })())JS", [this](auto evaluation){if(auto* e=evaluation.getError())finish(false,e->message);});
        } else if (stage == 2 && (int)state["composition"]["settings"]["voices"] == 8) {
            for (int i=5;i<8;i++) if ((int)state["plugin"]["channels"][i] != 1) { finish(false, "new voice channel"); return; }
            stage = 3;
            juce::MemoryBlock saved;
            processor.getStateInformation(saved);
            processor.setStateInformation(saved.getData(), (int)saved.getSize());
        } else if (stage == 3) {
            stage = 4;
            browser->evaluateJavascript(R"JS((()=>{
              if(!document.getElementById('legend').textContent.includes('Voice 8 (Upper Texture)'))throw Error('eight voice labels');
              if(document.querySelectorAll('#nmgRoutes .inactive-voice').length)throw Error('active voice display');
              return true;
            })())JS", [this](auto evaluation){if(auto* e=evaluation.getError())finish(false,e->message);else {
              stage=5;
              editor->setSize(1460,940);
              browser->evaluateJavascript(R"JS((()=>{
                for(const id of ['nmgOutput','nmgScale','nmgSync','nmgTheme'])if(!document.getElementById(id).closest('header'))throw Error('header placement '+id);
                if(document.getElementById('volume'))throw Error('volume slider still present');
                const values=[...document.getElementById('transpose').options].map(o=>Number(o.value));
                if(!values.includes(0)||values.some((v,i)=>i&&values[i-1]<=v))throw Error('octave ordering');
                document.getElementById('nmgTheme').click();
                document.getElementById('themeHex').value='FFC100';
                document.getElementById('themeHex').dispatchEvent(new Event('change'));
                document.getElementById('themeClose').click();
                const data=nmgBridge.hooks.midi();
                return JSON.stringify({path:nmgBridge.dragPath,base64:btoa(Array.from(data,x=>String.fromCharCode(x)).join(''))});
              })())JS", [this](auto result){
                if(auto* error=result.getError()){finish(false,error->message);return;}
                const auto data=juce::JSON::parse(result.getResult()->toString());
                const juce::File file(data["path"].toString());juce::MemoryBlock actual;juce::MemoryOutputStream expected;
                if(!file.loadFileAsData(actual)||!juce::Base64::convertFromBase64(expected,data["base64"].toString())||actual.getSize()!=expected.getDataSize()||std::memcmp(actual.getData(),expected.getData(),actual.getSize())!=0){finish(false,"drag file differs from original MIDI");return;}
                stage=6;
              });
            }});
        } else if(stage==6 && state["plugin"]["theme"].toString()=="FFC100") {
            editor.reset();editor.reset(processor.createEditor());
            if(editor->getWidth()!=1460||editor->getHeight()!=940){finish(false,"reopened size "+juce::String(editor->getWidth())+" x "+juce::String(editor->getHeight()));return;}
            editor->addToDesktop(juce::ComponentPeer::windowIsTemporary);editor->setVisible(true);
            browser=dynamic_cast<juce::WebBrowserComponent*>(editor->getChildComponent(0));stage=7;ticks=0;
        } else if(stage==7 && ++ticks>15){
            stage=8;
            browser->evaluateJavascript(R"JS((()=>{
              if(getComputedStyle(document.documentElement).getPropertyValue('--accent').trim()!=='#FFC100')throw Error('theme restore');
              if(document.querySelectorAll('#nmgRoutes select').length!==8)throw Error('reopen routing');
 if(getComputedStyle(document.getElementById('generate')).backgroundColor!=='rgb(255, 193, 0)')throw Error('theme button color '+getComputedStyle(document.getElementById('generate')).backgroundColor);
              return true;
            })())JS",[this](auto r){if(auto* e=r.getError())finish(false,e->message);else snapshotEditor(*editor,juce::File::getCurrentWorkingDirectory().getChildFile("build-mac/editor-wide.png").getFullPathName(),[this](bool ok){
 if(!ok){finish(false,"snapshot");return;}
 editor->setSize(760,850);stage=9;ticks=0;
 });});
        } else if(stage==9 && ticks>10){
            stage=10;
            browser->evaluateJavascript("document.getElementById('nmgTheme').click();true",[this](auto){stage=11;ticks=0;});
        } else if(stage==11 && ticks>10){
            stage=12;
            snapshotEditor(*editor,juce::File::getCurrentWorkingDirectory().getChildFile("build-mac/editor-picker.png").getFullPathName(),[this](bool ok){finish(ok,"editor defaults, labels, remap, header, theme/reopen, descending octaves, removed volume, byte-identical drag MIDI, persistent window size and snapshots");});
        }
    }
};
int main(){juce::ScopedJuceInitialiser_GUI initialiser;EditorCheck check;juce::MessageManager::getInstance()->runDispatchLoop();return check.result;}
