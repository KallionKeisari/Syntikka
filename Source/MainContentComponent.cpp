/*
  ==============================================================================

    MainContentComponent.cpp
    Created: 14 Nov 2023 12:04:50pm
    Author:  Henry Ala-Turkia

  ==============================================================================
*/

#include "MainContentComponent.h"


    juce::MidiKeyboardState keyboardState;
    SynthAudioSource synthAudioSource;
    juce::MidiKeyboardComponent keyboardComponent;
 
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};

    MainContentComponent()
        : synthAudioSource  (keyboardState),
          keyboardComponent (keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
        addAndMakeVisible (keyboardComponent);
        setAudioChannels (0, 2);
 
        setSize (600, 160);
        startTimer (400);
    }
    
    void timerCallback() override
    {
        keyboardComponent.grabKeyboardFocus();
        stopTimer();
    }
