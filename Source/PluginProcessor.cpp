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
    stretch_processor.smooth_reset(stretch::smooth_target);
    stretch_processor.setup(getNumInputChannels());
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
    
    //it'd be a bit better if it set the bpm only whenever it changed.
    double bpm = 120;
    //if (auto bpmFromHost = *getPlayHead()->getPosition()->getBpm())
    //    bpm = bpmFromHost;

    //same here but for any param
    stretch_processor.set_params(apvts, bpm);

    if (trigger) {
        stretch_processor.fill_buffer(buffer);
        stretch_processor.process(buffer);
    } else if (stretch_processor.buffer_is_dirty) stretch_processor.clear_buffer(getNumInputChannels());

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
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void StretchAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
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
