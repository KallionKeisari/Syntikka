#pragma once

//==============================================================================
template <typename Type>
class CustomOscillator
{
public:
    //==============================================================================
    CustomOscillator()
    {
        auto& osc = processorChain.template get<oscIndex>();
        osc.initialise ([] (Type x)
        {
            return juce::jmap (x,
                               Type (-juce::MathConstants<double>::pi),
                               Type (juce::MathConstants<double>::pi),
                               Type (-1),
                               Type (1));
        }, 2);
    }

    //==============================================================================
    void setFrequency (Type newValue, bool force = false)
    {
        auto& osc = processorChain.template get<oscIndex>();
        osc.setFrequency (newValue, force);     // [7]
    }

    //==============================================================================
    void setLevel (Type newValue)
    {
        auto& gain = processorChain.template get<gainIndex>();
        gain.setGainLinear (newValue);          // [8]
    }

    //==============================================================================
    void reset() noexcept
    {
         processorChain.reset(); // [4]
    }

    //==============================================================================
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        processorChain.process (context);       // [9]
    }

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        processorChain.prepare (spec); // [3]
    }

private:
    //==============================================================================
    enum
    {
        oscIndex,
        gainIndex   // [2]
    };

    juce::dsp::ProcessorChain<juce::dsp::Oscillator<Type>, juce::dsp::Gain<Type>> processorChain; // [1]
};

//==============================================================================
class Voice  : public juce::MPESynthesiserVoice
{
public:
    Voice()
    {
        auto& masterGain = processorChain.get<masterGainIndex>();
        masterGain.setGainLinear (0.7f);

        auto& filter = processorChain.get<filterIndex>();
        filter.setCutoffFrequencyHz (1000.0f);          // [3]
        filter.setResonance (0.7f);                     // [4]
        lfo.initialise ([] (float x) { return std::sin(x); }, 128);
        lfo.setFrequency (3.0f);

    }

    void setADSRParameters(float newAttack, float newDecay, float newSustain, float newRelease) {
        adsr.setParameters({ newAttack, newDecay, newSustain, newRelease });
    }

    // Getter methods for ADSR parameters
    float getAttack() const { return attack; }
    float getDecay() const { return decay; }
    float getSustain() const { return sustain; }
    float getRelease() const { return release; }

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        tempBlock = juce::dsp::AudioBlock<float> (heapBlock, spec.numChannels, spec.maximumBlockSize);
        processorChain.prepare (spec);

        lfo.prepare ({ spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels }); // [4]
    }
    

    //==============================================================================
    void noteStarted() override
    {
        auto velocity = getCurrentlyPlayingNote().noteOnVelocity.asUnsignedFloat();
        auto freqHz = (float) getCurrentlyPlayingNote().getFrequencyInHertz();

        processorChain.get<osc1Index>().setFrequency (freqHz, true);
        processorChain.get<osc1Index>().setLevel (velocity);

        processorChain.get<osc2Index>().setFrequency (freqHz * 1.01f, true);    // [3]
        processorChain.get<osc2Index>().setLevel (velocity);                    // [4]
        
        setADSRParameters(attack, decay, sustain, release);
    }

    //==============================================================================
    void notePitchbendChanged() override
    {
        auto freqHz = (float) getCurrentlyPlayingNote().getFrequencyInHertz();
        processorChain.get<osc1Index>().setFrequency (freqHz);
        processorChain.get<osc2Index>().setFrequency (freqHz * 1.01f);          // [5]
    }

    //==============================================================================
    void noteStopped (bool) override
    {
        clearCurrentNote();
    }

    //==============================================================================
    void notePressureChanged() override {}
    void noteTimbreChanged()   override {}
    void noteKeyStateChanged() override {}

    //==============================================================================
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        auto output = tempBlock.getSubBlock (0, (size_t) numSamples);
        output.clear();

        for (size_t pos = 0; pos < (size_t) numSamples;)
        {
            auto max = juce::jmin ((size_t) numSamples - pos, lfoUpdateCounter);
            auto block = output.getSubBlock (pos, max);

            juce::dsp::ProcessContextReplacing<float> context (block);
            processorChain.process (context);

            pos += max;
            lfoUpdateCounter -= max;

            if (lfoUpdateCounter == 0)
            {
                lfoUpdateCounter = lfoUpdateRate;
                auto lfoOut = lfo.processSample (0.0f);                                 // [5]
                auto curoffFreqHz = juce::jmap (lfoOut, -1.0f, 1.0f, 100.0f, 2000.0f);  // [6]
                processorChain.get<filterIndex>().setCutoffFrequencyHz (curoffFreqHz);  // [7]
            }
        }

        juce::dsp::AudioBlock<float> (outputBuffer)
            .getSubBlock ((size_t) startSample, (size_t) numSamples)
            .add (tempBlock);
    }

private:
    //==============================================================================
    juce::HeapBlock<char> heapBlock;
    juce::dsp::AudioBlock<float> tempBlock;

    juce::ADSR adsr; // Declare an ADSR envelope
    float attack { 0.1f };
    float decay { 0.1f };
    float sustain { 1.0f };
    float release { 0.5f };

    enum
    {
        osc1Index,
        osc2Index,
        filterIndex,        // [2]
        masterGainIndex
    };

    juce::dsp::ProcessorChain<CustomOscillator<float>, CustomOscillator<float>,
                              juce::dsp::LadderFilter<float>, juce::dsp::Gain<float>> processorChain; // [1]

    static constexpr size_t lfoUpdateRate = 100;
    size_t lfoUpdateCounter = lfoUpdateRate;
    juce::dsp::Oscillator<float> lfo;   // [1]
};

//==============================================================================
class AudioEngine  : public juce::MPESynthesiser
{
public:
    static constexpr auto maxNumVoices = 4;

    //==============================================================================
    AudioEngine()
    {
        for (auto i = 0; i < maxNumVoices; ++i)
            addVoice (new Voice);

        setVoiceStealingEnabled (true);
    }

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec) noexcept
    {
        setCurrentPlaybackSampleRate (spec.sampleRate);

        for (auto* v : voices)
            dynamic_cast<Voice*> (v)->prepare (spec);

        fxChain.prepare (spec);     // [3]
    }

private:
    //==============================================================================
    void renderNextSubBlock (juce::AudioBuffer<float>& outputAudio, int startSample, int numSamples) override
    {
        MPESynthesiser::renderNextSubBlock (outputAudio, startSample, numSamples);

        auto block = juce::dsp::AudioBlock<float> (outputAudio);                            // [4]
        auto blockToUse = block.getSubBlock ((size_t) startSample, (size_t) numSamples);    // [5]
        auto contextToUse = juce::dsp::ProcessContextReplacing<float> (blockToUse);         // [6]
        fxChain.process (contextToUse);                                                     // [7]
    }

    enum
    {
        reverbIndex // [2]
    };

    juce::dsp::ProcessorChain<juce::dsp::Reverb> fxChain;   // [1]
};

//==============================================================================
template <typename SampleType>
class AudioBufferQueue
{
public:
    //==============================================================================
    static constexpr size_t order = 9;
    static constexpr size_t bufferSize = 1U << order;
    static constexpr size_t numBuffers = 5;

    //==============================================================================
    void push (const SampleType* dataToPush, size_t numSamples)
    {
        jassert (numSamples <= bufferSize);

        int start1, size1, start2, size2;
        abstractFifo.prepareToWrite (1, start1, size1, start2, size2);

        jassert (size1 <= 1);
        jassert (size2 == 0);

        if (size1 > 0)
            juce::FloatVectorOperations::copy (buffers[(size_t) start1].data(), dataToPush, (int) juce::jmin (bufferSize, numSamples));

        abstractFifo.finishedWrite (size1);
    }

    //==============================================================================
    void pop (SampleType* outputBuffer)
    {
        int start1, size1, start2, size2;
        abstractFifo.prepareToRead (1, start1, size1, start2, size2);

        jassert (size1 <= 1);
        jassert (size2 == 0);

        if (size1 > 0)
            juce::FloatVectorOperations::copy (outputBuffer, buffers[(size_t) start1].data(), (int) bufferSize);

        abstractFifo.finishedRead (size1);
    }

private:
    //==============================================================================
    juce::AbstractFifo abstractFifo { numBuffers };
    std::array<std::array<SampleType, bufferSize>, numBuffers> buffers;
};

//==============================================================================
template <typename SampleType>
class ScopeDataCollector
{
public:
    //==============================================================================
    ScopeDataCollector (AudioBufferQueue<SampleType>& queueToUse)
        : audioBufferQueue (queueToUse)
    {}

    //==============================================================================
    void process (const SampleType* data, size_t numSamples)
    {
        size_t index = 0;

        if (state == State::waitingForTrigger)
        {
            while (index++ < numSamples)
            {
                auto currentSample = *data++;

                if (currentSample >= triggerLevel && prevSample < triggerLevel)
                {
                    numCollected = 0;
                    state = State::collecting;
                    break;
                }

                prevSample = currentSample;
            }
        }

        if (state == State::collecting)
        {
            while (index++ < numSamples)
            {
                buffer[numCollected++] = *data++;

                if (numCollected == buffer.size())
                {
                    audioBufferQueue.push (buffer.data(), buffer.size());
                    state = State::waitingForTrigger;
                    prevSample = SampleType (100);
                    break;
                }
            }
        }
    }

private:
    //==============================================================================
    AudioBufferQueue<SampleType>& audioBufferQueue;
    std::array<SampleType, AudioBufferQueue<SampleType>::bufferSize> buffer;
    size_t numCollected;
    SampleType prevSample = SampleType (100);

    static constexpr auto triggerLevel = SampleType (0.05);

    enum class State { waitingForTrigger, collecting } state { State::waitingForTrigger };
};

//==============================================================================
template <typename SampleType>
class ScopeComponent  : public juce::Component,
                        private juce::Timer
{
public:
    using Queue = AudioBufferQueue<SampleType>;

    //==============================================================================
    ScopeComponent (Queue& queueToUse)
        : audioBufferQueue (queueToUse)
    {
        sampleData.fill (SampleType (0));
        setFramesPerSecond (30);
    }

    //==============================================================================
    void setFramesPerSecond (int framesPerSecond)
    {
        jassert (framesPerSecond > 0 && framesPerSecond < 1000);
        startTimerHz (framesPerSecond);
    }

    //==============================================================================
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black);
        g.setColour (juce::Colours::white);

        auto area = getLocalBounds();
        auto h = (SampleType) area.getHeight();
        auto w = (SampleType) area.getWidth();

        // Oscilloscope
        auto scopeRect = juce::Rectangle<SampleType> { SampleType (0), SampleType (0), w, h / 2 };
        plot (sampleData.data(), sampleData.size(), g, scopeRect, SampleType (1), h / 4);

        // Spectrum
        auto spectrumRect = juce::Rectangle<SampleType> { SampleType (0), h / 2, w, h / 2 };
        plot (spectrumData.data(), spectrumData.size() / 4, g, spectrumRect);
    }

    //==============================================================================
    void resized() override {}

private:
    //==============================================================================
    Queue& audioBufferQueue;
    std::array<SampleType, Queue::bufferSize> sampleData;

    juce::dsp::FFT fft { Queue::order };
    using WindowFun = juce::dsp::WindowingFunction<SampleType>;
    WindowFun windowFun { (size_t) fft.getSize(), WindowFun::hann };
    std::array<SampleType, 2 * Queue::bufferSize> spectrumData;

    //==============================================================================
    void timerCallback() override
    {
        audioBufferQueue.pop (sampleData.data());
        juce::FloatVectorOperations::copy (spectrumData.data(), sampleData.data(), (int) sampleData.size());

        auto fftSize = (size_t) fft.getSize();

        jassert (spectrumData.size() == 2 * fftSize);
        windowFun.multiplyWithWindowingTable (spectrumData.data(), fftSize);
        fft.performFrequencyOnlyForwardTransform (spectrumData.data());

        static constexpr auto mindB = SampleType (-160);
        static constexpr auto maxdB = SampleType (0);

        for (auto& s : spectrumData)
            s = juce::jmap (juce::jlimit (mindB, maxdB, juce::Decibels::gainToDecibels (s) - juce::Decibels::gainToDecibels (SampleType (fftSize))), mindB, maxdB, SampleType (0), SampleType (1));

        repaint();
    }

    //==============================================================================
    static void plot (const SampleType* data,
                      size_t numSamples,
                      juce::Graphics& g,
                      juce::Rectangle<SampleType> rect,
                      SampleType scaler = SampleType (1),
                      SampleType offset = SampleType (0))
    {
        auto w = rect.getWidth();
        auto h = rect.getHeight();
        auto right = rect.getRight();

        auto center = rect.getBottom() - offset;
        auto gain = h * scaler;

        for (size_t i = 1; i < numSamples; ++i)
            g.drawLine ({ juce::jmap (SampleType (i - 1), SampleType (0), SampleType (numSamples - 1), SampleType (right - w), SampleType (right)),
                          center - gain * data[i - 1],
                          juce::jmap (SampleType (i), SampleType (0), SampleType (numSamples - 1), SampleType (right - w), SampleType (right)),
                          center - gain * data[i] });
    }
};

//==============================================================================
class SynthAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SynthAudioProcessor()
         : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    {}

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        audioEngine.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 2 });
        midiMessageCollector.reset (sampleRate);
    }

    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        // This is the place where you check if the layout is supported.
        // In this template code we only support mono or stereo.
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
         && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;

        return true;
    }

    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override
    {
        juce::ScopedNoDenormals noDenormals;
        auto totalNumInputChannels  = getTotalNumInputChannels();
        auto totalNumOutputChannels = getTotalNumOutputChannels();

        midiMessageCollector.removeNextBlockOfMessages (midiMessages, buffer.getNumSamples());

        for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
            buffer.clear (i, 0, buffer.getNumSamples());

        audioEngine.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
        scopeDataCollector.process (buffer.getReadPointer (0), (size_t) buffer.getNumSamples());
    }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override
    {
        return new SynthAudioProcessorEditor (*this);
    }

    bool hasEditor() const override                                        { return true; }

    //==============================================================================
    const juce::String getName() const override                            { return JucePlugin_Name; }
    bool acceptsMidi() const override                                      { return true; }
    bool producesMidi() const override                                     { return false; }
    bool isMidiEffect() const override                                     { return false; }
    double getTailLengthSeconds() const override                           { return 0.0; }

    //==============================================================================
    int getNumPrograms() override                                          { return 1; }
    int getCurrentProgram() override                                       { return 0; }
    void setCurrentProgram (int) override                                  {}
    const juce::String getProgramName (int) override                       { return {}; }
    void changeProgramName (int, const juce::String&) override             {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock&) override                 {}
    void setStateInformation (const void*, int) override                   {}

    //==============================================================================
    juce::MidiMessageCollector& getMidiMessageCollector() noexcept { return midiMessageCollector; }
    AudioBufferQueue<float>& getAudioBufferQueue() noexcept        { return audioBufferQueue; }

    float getAttack() const { return attack; }
    void setAttack(float newAttack) { attack = newAttack; }

    float getDecay() const { return decay; }
    void setDecay(float newDecay) { decay = newDecay; }

    float getSustain() const { return sustain; }
    void setSustain(float newSustain) { sustain = newSustain; }

    float getRelease() const { return release; }
    void setRelease(float newRelease) { release = newRelease; }

private:
    //==============================================================================

    float attack = 0.1f;
    float decay = 0.1f;
    float sustain = 1.0f;
    float release = 0.5f;


    class SynthAudioProcessorEditor  : public juce::AudioProcessorEditor
    {
    public:
        SynthAudioProcessorEditor (SynthAudioProcessor& p)
            : AudioProcessorEditor (&p),
              dspProcessor (p),
              scopeComponent (dspProcessor.getAudioBufferQueue())
        {
            addAndMakeVisible (midiKeyboardComponent);
            addAndMakeVisible (scopeComponent);

            addAndMakeVisible (attackSlider);
            attackSlider.setSliderStyle (juce::Slider::LinearHorizontal);
            attackSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 50, 20);
            attackSlider.setRange (0.0, 5.0, 0.1);
            attackSlider.setValue(dspProcessor.getAttack());

            addAndMakeVisible (decaySlider);
            decaySlider.setSliderStyle (juce::Slider::LinearHorizontal);
            decaySlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 50, 20);
            decaySlider.setRange (0.0, 5.0, 0.1);
            decaySlider.setValue(dspProcessor.getDecay());

            addAndMakeVisible (sustainSlider);
            sustainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
            sustainSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 50, 20);
            sustainSlider.setRange (0.0, 5.0, 0.1);
            sustainSlider.setValue(dspProcessor.getSustain());

            addAndMakeVisible (releaseSlider);
            releaseSlider.setSliderStyle (juce::Slider::LinearHorizontal);
            releaseSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 50, 20);
            releaseSlider.setRange (0.0, 5.0, 0.1);
            releaseSlider.setValue(dspProcessor.getRelease());

            attackSlider.onValueChange = [this] { dspProcessor.setAttack(attackSlider.getValue()); };
            decaySlider.onValueChange = [this] { dspProcessor.setDecay(decaySlider.getValue()); };
            sustainSlider.onValueChange = [this] { dspProcessor.setSustain(sustainSlider.getValue()); };
            releaseSlider.onValueChange = [this] { dspProcessor.setRelease(releaseSlider.getValue()); };


            setSize (400, 300);

            auto area = getLocalBounds();
            scopeComponent.setTopLeftPosition (0, 80);
            scopeComponent.setSize (area.getWidth(), area.getHeight() - 100);

            midiKeyboardComponent.setMidiChannel (2);
            midiKeyboardState.addListener (&dspProcessor.getMidiMessageCollector());
        }

        ~SynthAudioProcessorEditor() override
        {
            midiKeyboardState.removeListener (&dspProcessor.getMidiMessageCollector());
        }

        //==============================================================================
        void paint (juce::Graphics& g) override
        {
            g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
        }

        void resized() override
        {
            auto area = getLocalBounds();
            midiKeyboardComponent.setBounds (area.removeFromTop (80).reduced (8));
            
            // Set bounds for sliders
            const int sliderWidth = 200;
            const int sliderHeight = 20;
            const int sliderMargin = 10;

            attackSlider.setBounds(area.removeFromTop(sliderHeight).withSizeKeepingCentre(sliderWidth, sliderHeight));
            area.removeFromTop(sliderMargin); // Add margin between sliders

            decaySlider.setBounds(area.removeFromTop(sliderHeight).withSizeKeepingCentre(sliderWidth, sliderHeight));
            area.removeFromTop(sliderMargin);

            sustainSlider.setBounds(area.removeFromTop(sliderHeight).withSizeKeepingCentre(sliderWidth, sliderHeight));
            area.removeFromTop(sliderMargin);

            releaseSlider.setBounds(area.removeFromTop(sliderHeight).withSizeKeepingCentre(sliderWidth, sliderHeight));
        }

    private:
        //==============================================================================
        SynthAudioProcessor& dspProcessor;

        juce::Slider attackSlider;
        juce::Slider decaySlider;
        juce::Slider sustainSlider;
        juce::Slider releaseSlider;
        juce::MidiKeyboardState midiKeyboardState;
        juce::MidiKeyboardComponent midiKeyboardComponent { midiKeyboardState, juce::MidiKeyboardComponent::horizontalKeyboard };
        ScopeComponent<float> scopeComponent;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthAudioProcessorEditor)
    };

    //==============================================================================
    AudioEngine audioEngine;
    juce::MidiMessageCollector midiMessageCollector;
    AudioBufferQueue<float> audioBufferQueue;
    ScopeDataCollector<float> scopeDataCollector { audioBufferQueue };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthAudioProcessor)
};
