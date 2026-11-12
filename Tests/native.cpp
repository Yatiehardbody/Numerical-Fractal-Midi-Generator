#include "Plugin.h"
#include <iostream>
#include <stdexcept>

static void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
static juce::var fixture(bool sync) {
    auto value = juce::JSON::parse(R"({"settings":{"voices":5},"plugin":{"channels":[1,1,1,1,1,1,1,1],"sync":false,"external":true,"scale":125},"composition":{"settings":{"bpm":120},"maxBeat":4,"notes":[{"startBeat":0,"durationBeats":1,"midiNote":60,"velocity":90,"track":0},{"startBeat":0.5,"durationBeats":1,"midiNote":60,"velocity":80,"track":1}]}})");
    value["plugin"].getDynamicObject()->setProperty("sync",sync);
    return value;
}
struct PlayHead : juce::AudioPlayHead {
    double beat=0, bpm=120;bool playing=true;
    juce::Optional<PositionInfo> getPosition()const override {PositionInfo p;p.setPpqPosition(beat);p.setBpm(bpm);p.setIsPlaying(playing);return p;}
};
int main() {
    try {
        juce::ScopedJuceInitialiser_GUI init;
        NMGProcessor p;p.prepareToPlay(48000,512);p.rememberEditorSize(1440,900);p.acceptState(fixture(false));
        juce::MemoryBlock blob;p.getStateInformation(blob);
        NMGProcessor restored;restored.setStateInformation(blob.getData(),(int)blob.getSize());
        require(juce::JSON::toString(p.getState())==juce::JSON::toString(restored.getState()),"state roundtrip");
        require(restored.editorSize()==juce::Point<int>(1440,900),"window dimensions survive state and UI updates");restored.prepareToPlay(48000,512);restored.command(juce::JSON::parse("{\"type\":\"play\"}"));
        juce::AudioBuffer<float> audio(2,512);juce::MidiBuffer midi;int ons=0,offs=0;
        for(int i=0;i<200;i++){restored.processBlock(audio,midi);for(const auto m:midi){auto message=m.getMessage();if(message.isNoteOn())ons++;if(message.isNoteOff())offs++;require(message.getChannel()==1,"default channel");require(!message.isProgramChange(),"no program changes");}}
        require(ons==1&&offs==1,"shared pitch gate");
        NMGProcessor host;PlayHead transport;host.setPlayHead(&transport);host.prepareToPlay(48000,512);host.acceptState(fixture(true));
        host.processBlock(audio,midi);require(midi.getNumEvents()==1,"host start");
        auto source=host.getState();auto originalNotes=juce::JSON::toString(source["composition"]["notes"]);
        source["plugin"]["channels"].getArray()->set(0,16);host.acceptState(source);transport.beat=512.0/24000;
        host.processBlock(audio,midi);bool released=false,routed=false;for(const auto m:midi){auto message=m.getMessage();released|=message.isNoteOff()&&message.getChannel()==1;routed|=message.isNoteOn()&&message.getChannel()==16;}
        require(released&&routed,"live remapping releases old channel and resumes new channel");
        require(juce::JSON::toString(host.getState()["composition"]["notes"])==originalNotes,"immutable musical events");
        source=host.getState();source["plugin"].getDynamicObject()->setProperty("scale",150);host.acceptState(source);transport.beat+=512.0/24000;
        host.processBlock(audio,midi);require(midi.getNumEvents()==0,"UI scaling must not retrigger notes");
        transport.beat=.5;transport.bpm=240;host.processBlock(audio,midi);require(host.hostBpm==240,"host tempo follow");
        transport.playing=false;host.processBlock(audio,midi);require(midi.getNumEvents()==2,"host stop releases both channels");
        for(const auto m:midi)require(m.getMessage().isNoteOff(),"stop messages");
        std::cout<<"PASS native processor: state roundtrip, shared gates, live remap, unchanged notes, host tempo/seek/stop, UI edits without retrigger\n";
        return 0;
    } catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
