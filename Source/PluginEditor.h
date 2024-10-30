/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class Sand_stretch_remakeAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    Sand_stretch_remakeAudioProcessorEditor (Sand_stretch_remakeAudioProcessor&);
    ~Sand_stretch_remakeAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Sand_stretch_remakeAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sand_stretch_remakeAudioProcessorEditor)
};
