/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

//==============================================================================
sand_stretchAudioProcessor::sand_stretchAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : stretch_processor(), AudioProcessor(BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#else
: AudioProcessor()
#endif
, parameters(*this, nullptr, juce::Identifier("sand_stretch"),
    {
        std::make_unique<juce::AudioParameterBool>("triggerParameter", "Trigger", false),
        std::make_unique<juce::AudioParameterBool>("holdParameter", "Hold", false),
        std::make_unique<juce::AudioParameterBool>("reverseParameter", "Reverse", false),
        std::make_unique<juce::AudioParameterBool>("removeDcOffsetParameter", "Reduce DC Offset", true),
        std::make_unique<juce::AudioParameterChoice>("declickParameter", "Declick", juce::StringArray{"Off", "32", "64", "128", "256", "512"}, 0),
        std::make_unique<juce::AudioParameterFloat>("ratioParameter", "Ratio", 1.0f, 12.0f, 2.0f),
        std::make_unique<juce::AudioParameterInt>("samplesParameter", "Samples", 16, 16384, 128),
        //skip samples minimum value could be set to like 0.01, but it crashed on me once so proceed with caution.
        std::make_unique<juce::AudioParameterFloat>("skipSamplesParameter", "[DANGEROUS] Skip Samples", 1, 16, 1), 
        std::make_unique<juce::AudioParameterFloat>("crossfadeParameter", "Crossfade", 0.0f, 100.0f, 0.0f),
        std::make_unique<juce::AudioParameterInt>("holdOffsetParameter", "Hold Offset", 0, 4096, 0),
        std::make_unique<juce::AudioParameterInt>("zcrossParameter", "zCross Window Size", 0, 64, 0),
        std::make_unique<juce::AudioParameterInt>("zcrossOffsetParameter", "zCross Window Offset", 0, 1024, 0),
        std::make_unique<juce::AudioParameterInt>("bufferSizeParameter", "bufferSize" , 1, 30, 8),
    })
{

    triggerParameter = parameters.getRawParameterValue("triggerParameter");
    holdParameter = parameters.getRawParameterValue("holdParameter");
    reverseParameter = parameters.getRawParameterValue("reverseParameter");
    removeDcOffsetParameter = parameters.getRawParameterValue("removeDcOffsetParameter");
    declickParameter = parameters.getRawParameterValue("declickParameter");
    ratioParameter = parameters.getRawParameterValue("ratioParameter");
    samplesParameter = parameters.getRawParameterValue("samplesParameter");
    skipSamplesParameter = parameters.getRawParameterValue("skipSamplesParameter");
    crossfadeParameter = parameters.getRawParameterValue("crossfadeParameter");
    holdOffsetParameter = parameters.getRawParameterValue("holdOffsetParameter");
    zcrossParameter = parameters.getRawParameterValue("zcrossParameter");
    zcrossOffsetParameter = parameters.getRawParameterValue("zcrossOffsetParameter");
    bufferSizeParameter = parameters.getRawParameterValue("bufferSizeParameter");
    
}

sand_stretchAudioProcessor::~sand_stretchAudioProcessor()
{
}

//==============================================================================
const juce::String sand_stretchAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool sand_stretchAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool sand_stretchAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool sand_stretchAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double sand_stretchAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int sand_stretchAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int sand_stretchAudioProcessor::getCurrentProgram()
{
    return 0;
}

void sand_stretchAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String sand_stretchAudioProcessor::getProgramName (int index)
{
    return {};
}

void sand_stretchAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void sand_stretchAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    stretch_processor.num_input_channels = getNumInputChannels();
    stretch_processor.sample_rate = sampleRate;
    stretch_processor.setup_arrays();
}

void sand_stretchAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool sand_stretchAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void sand_stretchAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    stretch_processor.process(buffer, parameters);

}

//==============================================================================
bool sand_stretchAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* sand_stretchAudioProcessor::createEditor()
{
    //i know this is probably not the best way to resize the editor in order to fit all the parameters,
    //but i don't know how to do it otherwise and it works so whatever
    juce::GenericAudioProcessorEditor* genericEditor = new juce::GenericAudioProcessorEditor(*this);
    genericEditor->setSize(genericEditor->getWidth(), genericEditor->getHeight()+100);
    return genericEditor;
}

//==============================================================================
void sand_stretchAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void sand_stretchAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    xmlState->setAttribute("triggerParameter", 0);
    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName(parameters.state.getType())) {
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new sand_stretchAudioProcessor();
}
