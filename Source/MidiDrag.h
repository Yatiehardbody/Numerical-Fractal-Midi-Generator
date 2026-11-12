#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
// Transparent native hit region over the HTML button. macOS drag starts directly
// from mouseDragged, avoiding asynchronous JavaScript/file-promise drag failures.
class NativeMidiDrag {
public:
 NativeMidiDrag(); ~NativeMidiDrag();
 void place(juce::Component&, juce::Rectangle<int>, bool);
 void setFile(const juce::File&);
private: struct Impl; std::unique_ptr<Impl> impl;
};
