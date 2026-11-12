# Windows x64 build

The GitHub Actions Windows workflow downloads JUCE 8.0.12 and WebView2, regenerates the embedded interface, checks all 78 original MIDI/event fixtures, builds the VST3 with Visual Studio, runs native tests, and packages an Inno Setup installer with an uninstaller.

Download the NMG-1.1.1-Windows-x64 workflow artifact after a successful run. The installation ZIP inside contains the Setup executable, complete VST3 bundle, Microsoft WebView2 bootstrapper, checksums and installation instructions.

The original HTML in Reference/Original.html includes the sampled instrument data and attribution. Its mathematical engine and chords remain unchanged.

Windows FL Studio interaction requires testing on the target computer. Compilation and automated tests do not establish DAW UI/drag-and-drop compatibility.
