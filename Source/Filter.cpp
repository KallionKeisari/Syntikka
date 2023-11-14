/*
  ==============================================================================

    Filter.cpp
    Created: 14 Nov 2023 8:26:21pm
    Author:  Nia Lehtonen

  ==============================================================================
*/

#include "Filter.h"

Filter::Filter()
    : IIRFilter()
{
    setCutoffFrequency(1000.0); // Default cutoff frequency (adjust as needed)
}

Filterr::~Filter()
{
}

void Filter::setCutoffFrequency(double newCutoffFrequency)
{
    cutoffFrequency = newCutoffFrequency;
    double sampleRate = AudioProcessor::getSampleRate();
    double frequency = cutoffFrequency / sampleRate;
    
    // Design a first-order low-pass filter
    // The coefficients are calculated using the setCoefficients() method
    setCoefficients(IIRCoefficients::makeLowPass(sampleRate, frequency));
}

void Filter::processBlock(AudioBuffer<float>& buffer)
{
    // Apply the filter to each channel in the audio buffer
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        // Apply the filter to the entire buffer
        processSamples(buffer.getWritePointer(channel), buffer.getNumSamples());
    }
}
