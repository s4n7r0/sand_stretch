/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class sand_stretchAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    sand_stretchAudioProcessor();
    ~sand_stretchAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    
    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    void insertToBuffer(float sample, int index, int channel);
    void clearBuffer();
    void setSamples();
    void setBufferSize();
    void setRatio();
    
private:

    struct channelStruct {
        int sampleIn = 0;
        float sampleOut = 0;
        float sampleOffset = 0;
    };

    juce::AudioProcessorValueTreeState parameters;

    enum enumZCROSSSTATE { NEGATIVE, NONE, POSITIVE };
    std::atomic<float>* triggerParameter = nullptr;
    std::atomic<float>* holdParameter = nullptr;
    std::atomic<float>* reverseParameter = nullptr;
    std::atomic<float>* removeDcOffsetParameter = nullptr;
    std::atomic<float>* samplesParameter = nullptr;
    std::atomic<float>* skipSamplesParameter = nullptr;
    std::atomic<float>* crossfadeParameter = nullptr;
    std::atomic<float>* holdOffsetParameter = nullptr;
    std::atomic<float>* bufferSizeParameter = nullptr;
    std::atomic<float>* ratioParameter = nullptr;
    //std::atomic<float>* multiplierParameter= nullptr;
    std::atomic<float>* zcrossParameter = nullptr;
    std::atomic<float>* zcrossOffsetParameter = nullptr;

    juce::SmoothedValue<float> smoothBegin;
    juce::SmoothedValue<float> smoothEnd;

    std::vector<float> bufferArray[2];
    //TODO: Dynamically adjust zCrossArray to fit all samples
    std::vector<float> zCrossArray;
    std::vector<channelStruct> channelSample;
    std::vector<float> previousSampleOffsets;

    bool stop_ = false;
    bool cleared_ = false;
    bool holding_ = false;
    bool zCrossing_ = false;
    bool ratioChanged_ = false;
    enumZCROSSSTATE zCrossState_ = NONE;

    int samplesSize_ = 256;
    int numInputChannels_ = 0;
    int bufferSize_ = 8;

    int maxSamplesInBuffer_ = 0;
    int sampleRate_ = 0;

    int ratioSamples_ = 1;
    int samplesBoundary_ = 0;

    float ratio_ = 1.0f;

    int zCrossWindow_ = 0;
    int zCrossOffset_ = 0;
    int zCrossHoldOffset_ = 0;
    bool zCrossHoldOffsetMoved_ = false;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sand_stretchAudioProcessor)
};
