/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "StretchProcessor.h"

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
    
private:

    stretch::StretchProcessor stretch_processor;

    juce::AudioProcessorValueTreeState parameters;

    std::atomic<float>* triggerParameter = nullptr;
    std::atomic<float>* holdParameter = nullptr;
    std::atomic<float>* reverseParameter = nullptr;
    std::atomic<float>* removeDcOffsetParameter = nullptr;
    std::atomic<float>* declickParameter = nullptr;
    std::atomic<float>* samplesParameter = nullptr;
    std::atomic<float>* skipSamplesParameter = nullptr;
    std::atomic<float>* crossfadeParameter = nullptr;
    std::atomic<float>* holdOffsetParameter = nullptr;
    std::atomic<float>* bufferSizeParameter = nullptr;
    std::atomic<float>* ratioParameter = nullptr;
    //std::atomic<float>* multiplierParameter= nullptr;
    std::atomic<float>* zcrossParameter = nullptr;
    std::atomic<float>* zcrossOffsetParameter = nullptr;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sand_stretchAudioProcessor)
};
