/*
  ==============================================================================

    Filter.h
    Created: 14 Nov 2023 8:26:30pm
    Author:  Nia Lehtonen

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class Filter : public IIRFilter {
  public:
    Filter();
    Filter();

    void setCutoffFrequency(double newCutoffFrequency);
    void processBlock(AudioBuffer<float>& buffer);

  private:
    double cutoffFrequency;
};
