#pragma once
#include <JuceHeader.h>
#include "Scheduler.h"
class NMGProcessor : public juce::AudioProcessor {
public:
 NMGProcessor();~NMGProcessor() override=default;
 void prepareToPlay(double,int) override;void releaseResources() override{}
 bool isBusesLayoutSupported(const BusesLayout&)const override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&)override;
 juce::AudioProcessorEditor* createEditor()override;bool hasEditor()const override{return true;}
 const juce::String getName()const override{return "NMG";}bool acceptsMidi()const override{return false;}bool producesMidi()const override{return true;}bool isMidiEffect()const override{return false;}double getTailLengthSeconds()const override{return 0;}
 int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}const juce::String getProgramName(int)override{return "Default";}void changeProgramName(int,const juce::String&)override{}
 void getStateInformation(juce::MemoryBlock&)override;void setStateInformation(const void*,int)override;
 void rememberEditorSize(int,int);juce::Point<int> editorSize();
 void acceptState(const juce::var&,bool preserveEditorSize=false);juce::var getState();void command(const juce::var&);void publish();
 std::atomic<bool> sync{true},external{false},play{false};std::atomic<double> displayedBeat{0},hostBpm{120};std::atomic<bool> hostRunning{false};std::atomic<uint64_t> stateRevision{0};
private:
 juce::CriticalSection stateLock;juce::var state;std::atomic<nmg::Score*> score{nullptr},hazard{nullptr};std::unique_ptr<nmg::Score> owned;std::vector<std::unique_ptr<nmg::Score>> retired;
 nmg::Scheduler scheduler;juce::MidiBuffer output;double sampleRate=44100,freeBeat=0;uint64_t handledSeek=0,handledMode=0;std::atomic<uint64_t> seekVersion{0},modeVersion{0};std::atomic<double> seekBeat{0};
};
