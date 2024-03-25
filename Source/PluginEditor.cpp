/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
sand_stretchAudioProcessorEditor::sand_stretchAudioProcessorEditor (sand_stretchAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
}

sand_stretchAudioProcessorEditor::~sand_stretchAudioProcessorEditor()
{
}

void sand_stretchAudioProcessorEditor::sliderValueChanged(juce::Slider* slider) 
{
}

//==============================================================================
void sand_stretchAudioProcessorEditor::paint (juce::Graphics& g)
{
}

void sand_stretchAudioProcessorEditor::resized()
{
}
