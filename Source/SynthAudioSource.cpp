/*
  ==============================================================================

    SynthAudioSource.cpp
    Created: 14 Nov 2023 12:19:14pm
    Author:  Henry Ala-Turkia

  ==============================================================================
*/

#include "SynthAudioSource.h"

class SynthAudioSource   : public juce::AudioSource
{
public:
    SynthAudioSource (juce::MidiKeyboardState& keyState)
        : keyboardState (keyState)
    {
        for (auto i = 0; i < 4; ++i)                // [1]
            synth.addVoice (new SineWaveVoice());
 
        synth.addSound (new SineWaveSound());       // [2]
    }
 
    void setUsingSineWaveSound()
    {
        synth.clearSounds();
    }
 
    void prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate) override
    {
        synth.setCurrentPlaybackSampleRate (sampleRate); // [3]
    }
 
    void releaseResources() override {}
 
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();
 
        juce::MidiBuffer incomingMidi;
        keyboardState.processNextMidiBuffer (incomingMidi, bufferToFill.startSample,
                                             bufferToFill.numSamples, true);       // [4]
 
        synth.renderNextBlock (*bufferToFill.buffer, incomingMidi,
                               bufferToFill.startSample, bufferToFill.numSamples); // [5]
    }
 
private:
    juce::MidiKeyboardState& keyboardState;
    juce::Synthesiser synth;
};
