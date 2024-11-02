/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
StretchAudioProcessor::StretchAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     :  undo(), apvts(*this, &undo, "PARAMETERS", stretch::create_layout()), stretch_processor(), AudioProcessor(BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

StretchAudioProcessor::~StretchAudioProcessor()
{
}

//==============================================================================
const juce::String StretchAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool StretchAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool StretchAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool StretchAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double StretchAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int StretchAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int StretchAudioProcessor::getCurrentProgram()
{
    return 0;
}

void StretchAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String StretchAudioProcessor::getProgramName (int index)
{
    return {};
}

void StretchAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void StretchAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    stretch_processor.num_channels = getNumInputChannels();
    stretch_processor.sample_rate = sampleRate;
    stretch_processor.setup();
}

void StretchAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool StretchAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void StretchAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    bool trigger = (bool)*apvts.getRawParameterValue("trigger");

    //for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    //    buffer.clear (i, 0, buffer.getNumSamples());

    //maybe passivelly fill the buffer?
    //eg fill buffer up to max grain_size * 2 and then increase limit if trigger is on

    if (trigger) {
        stretch_processor.fill_buffer(buffer);
        stretch_processor.process(buffer);
    } else if (stretch_processor.buffer_is_dirty) stretch_processor.clear_buffer();

}

//==============================================================================
bool StretchAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* StretchAudioProcessor::createEditor()
{
    return new StretchAudioProcessorEditor (*this);
}

//==============================================================================
void StretchAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void StretchAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StretchAudioProcessor();
}


int StretchAudioProcessor::get_editor_width() {
    auto size = apvts.state.getOrCreateChildWithName("lastSize", &undo);
    return size.getProperty("width", stretch::size_width);
}

int StretchAudioProcessor::get_editor_height() {
    auto size = apvts.state.getOrCreateChildWithName("lastSize", &undo);
    return size.getProperty("height", stretch::size_height);
}

void StretchAudioProcessor::set_editor_size(int width, int height) {
    auto size = apvts.state.getOrCreateChildWithName("lastSize", &undo);
    size.setProperty("width", width, &undo);
    size.setProperty("height", height, &undo);
}
