/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "StretchEditor.h"
using namespace stretch::components;
//==============================================================================
/**
*/
class StretchAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    StretchAudioProcessorEditor (StretchAudioProcessor&);
    ~StretchAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    inline void setup();

    void show_or_hide();

    void set_font_size(juce::Graphics&g, float size) {
        text_size = size;
        g.setFont(size);
    }
    
    void set_scales();
    void draw_labels(juce::Graphics&g);
    void draw_help(juce::Graphics&g);

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    StretchAudioProcessor& audioProcessor;

    float text_size;
    float x_scale;
    float y_scale;
    float abs_scale;
    float slider_width;

    std::vector<String> help_texts;
    bool help_state;

    std::vector<AttachedComponent*> components; // TODO: fix the leak

    stretch::StretchBounds grain_text_bounds;
    stretch::StretchBounds ratio_text_bounds;

    //stretch::URLTimer url_timer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StretchAudioProcessorEditor)
};
