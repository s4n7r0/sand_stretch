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
     : AudioProcessor (BusesProperties()
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
    sampleRate_ = sampleRate;
    numInputChannels_ = getTotalNumInputChannels();
    channelSample.resize(numInputChannels_);
    previousSampleOffsets.resize(numInputChannels_);
    setBufferSize();
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

    auto mainInputOutput = getBusBuffer(buffer, true, 0);

    auto trigger = triggerParameter->load();
    auto hold = holdParameter->load();
    auto reverse = reverseParameter->load();
    auto removeDcOffset = removeDcOffsetParameter->load();
    auto ratio = ratioParameter->load();
    auto samplesSize = samplesParameter->load();
    auto skipSamples = skipSamplesParameter->load();
    auto bufferSize = bufferSizeParameter->load();
    auto holdOffset = holdOffsetParameter->load();
    auto zCrossWindow = zcrossParameter->load();
    auto zCrossOffset = zcrossOffsetParameter->load();
    auto crossfade = crossfadeParameter->load()/200.0f; //dividing by 200 to get values in range 0-0.5

    if (samplesSize_ != samplesSize) {
        setSamples();
    }

    if (ratio_ != ratio) {
        setRatio();
    }

    if (bufferSize_ != bufferSize) {
        setBufferSize();
    }

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel) {

        bool dcOffsetFound = false;
        float dcOffsetSample{};

        auto sampleIn = buffer.getReadPointer(channel);
        auto sampleOut = buffer.getWritePointer(channel);

        int& currentSampleIn = channelSample[channel].sampleIn;
        float& currentSampleOut = channelSample[channel].sampleOut;
        float& currentSampleOffset = channelSample[channel].sampleOffset;

        if (trigger && hold && !holding_) {
            holding_ = true;
            setRatio();
        }

        if (!hold && holding_) {
            holding_ = false;
            if (zCrossing_) setSamples();
            zCrossing_ = false;
            zCrossHoldOffsetMoved_ = true;
            setRatio();
        }

        if (trigger) {

            cleared_ = false;

            for (auto sample = 0; sample < buffer.getNumSamples(); ++sample) {

                if (stop_) break;

                insertToBuffer(sampleIn[sample], currentSampleIn, channel);
                currentSampleIn++;

                if (currentSampleIn >= maxSamplesInBuffer_) break;
            }

            if (currentSampleIn <= samplesSize_) continue;

            for (auto sample = 0; sample < buffer.getNumSamples(); ++sample) {

                //zCross
                //channel needs to be 0 otherwise it may get desynced when hold and zcrossing was on before the trigger.
                if (holding_ && zCrossWindow > 0 && channel == 0 && zCrossWindow != zCrossWindow_ || zCrossOffset != zCrossOffset_) {

                    zCrossWindow_ = zCrossWindow;
                    zCrossOffset_ = zCrossOffset;

                    int offset = 0;
                    int windowAndOffset = zCrossWindow_ + zCrossOffset_;

                    //makes sure there are atleast 3 samples in the window, maybe useless idk.
                    while ((zCrossArray[windowAndOffset + offset] - zCrossArray[windowAndOffset - zCrossWindow_]) <= 3) {
                        offset++;
                        if (windowAndOffset + offset >= zCrossArray.size()) {
                            offset = 0;
                            break;
                        }
                        if (zCrossArray[windowAndOffset + offset] <= 1) break;
                    }

                    //get offset of where currently held sample is.
                    if (!zCrossHoldOffset_ || zCrossHoldOffsetMoved_) {
                        for (int i = 0; i < zCrossArray.size(); ++i) {
                            //sometimes it hangs up but resumes some time later
                            //probably something to do with floats
                            if (zCrossArray[i] <= 0) break;
                            if (zCrossArray[i] >= currentSampleOffset + zCrossArray[0]) {
                                zCrossHoldOffset_ = i;
                                zCrossHoldOffsetMoved_ = false;
                                break;
                            }
                        }
                    }

                    windowAndOffset += zCrossHoldOffset_;

                    samplesBoundary_ = zCrossArray[windowAndOffset + offset] - zCrossArray[windowAndOffset - zCrossWindow_];

                    if (!zCrossing_) {
                        for (int channel = 0; channel < numInputChannels_; ++channel) {
                            previousSampleOffsets[channel] = channelSample[channel].sampleOffset;
                        }
                        zCrossing_ = true;
                    }

                    for (int channel = 0; channel < numInputChannels_; ++channel) {
                        channelSample[channel].sampleOffset = zCrossArray[windowAndOffset];
                    }
                }

                int currentOutAndOffsetForwards = currentSampleOut + currentSampleOffset;
                int currentOutAndOffsetBackwards = currentSampleOffset + samplesBoundary_ * skipSamples - currentSampleOut - 1; 
                //subtracting by one so it doesn't go past the boundary.

                //this may desync the audio a bit.
                //if hold was on before the trigger, this may result in a negative index.
                if (currentOutAndOffsetBackwards < 0) {
                    for (int channel = 0; channel <= numInputChannels_ - 1; ++channel) {
                        channelSample[channel].sampleOut = 0;
                    }
                    //currentSampleOut = 0;
                    //sample--; //this is dangerous, but prevents desync in some cases.
                    continue;
                }

                int holding = holding_;

                int forwardsIndex = currentOutAndOffsetForwards + holdOffset * holding;
                int backwardsIndex = currentOutAndOffsetBackwards + holdOffset * holding;

                if (forwardsIndex >= maxSamplesInBuffer_) {
                    continue;
                }

                if (backwardsIndex >= maxSamplesInBuffer_) {
                    continue;
                }

                //reset hold to where it was before zcrossing
                if (holding_ && zCrossing_ && zCrossWindow == 0) {
                    zCrossing_ = false;

                    for (int channel = 0; channel < numInputChannels_; ++channel) {
                        channelSample[channel].sampleOffset = previousSampleOffsets[channel];
                        previousSampleOffsets[channel] = 0.f;
                    }

                    setSamples();
                }

                float outputForwards = bufferArray[channel][forwardsIndex];
                float outputBackwards = bufferArray[channel][backwardsIndex];

                if (reverse) {
                    std::swap(outputForwards, outputBackwards);
                }     
                
                //DC Offset
                if (removeDcOffset && !dcOffsetFound && samplesBoundary_ > 0) {

                    float average{};
                    float boundaryWithOffsetAndShit = (samplesBoundary_ * skipSamples) + currentSampleOffset + holdOffset ;

                    for (float i = currentSampleOffset + holdOffset; i < boundaryWithOffsetAndShit; i += 1 * skipSamples) {
                        if (i >= maxSamplesInBuffer_) break;
                        average += bufferArray[channel][i];
                    }

                    dcOffsetSample = std::clamp((average / (samplesBoundary_ * skipSamples )) * -1, -1.f, 1.f);

                    dcOffsetFound = true;
                }

                sampleOut[sample] = outputForwards + dcOffsetSample;

                //Crossfade
                if (crossfade >= 0.01f) {
                    outputForwards *= 1.0f - crossfade;
                    outputBackwards *= crossfade;
                    //this bad boy from https://signalsmith-audio.co.uk/writing/2021/cheap-energy-crossfade/
                    // doesn't really work but whatever
                    sampleOut[sample] = (outputForwards + outputBackwards) * (1 + 0.4186 * crossfade) + dcOffsetSample * 2 * crossfade;
                }

                currentSampleOut += 1 * skipSamples;

                int zCrossing = !zCrossing_;

                if (currentSampleOut >= samplesBoundary_ * skipSamples - ((samplesSize_ / ratio) * holding) * zCrossing) {
                    currentSampleOffset += ratioSamples_;
                    currentSampleOut = 0;
                }
            }
        }

        if (!trigger && !cleared_) {
            clearBuffer();
            cleared_ = true;
        }
    }
}

void sand_stretchAudioProcessor::insertToBuffer(float sample, int index, int channel) {
    //make sure samples from all channels are gathered.
    for (int i = 0; i < numInputChannels_; ++i) {
        if (channelSample[i].sampleIn >= maxSamplesInBuffer_) {
            if (i == numInputChannels_ - 1) stop_ = true;
            return;
        }
    }

    bufferArray[channel][index] = sample;

    //collect indexes of points where summed samples crossed zero.
    static int zCrossArrayCounter = 0;

    if (channel == numInputChannels_ - 1) {

        float summedToMonoSample{};

        for (int i = 0; i < numInputChannels_; ++i) {
            summedToMonoSample += bufferArray[i][index];
        }

        summedToMonoSample /= numInputChannels_;

        if (zCrossState_ == NONE) {
            zCrossArrayCounter = 0;
            if (summedToMonoSample > 0) {
                zCrossState_ = POSITIVE;
            }
            else {
                zCrossState_ = NEGATIVE;
            }
        }

        if (zCrossArrayCounter >= zCrossArray.size()) return;

        if (summedToMonoSample > 0 && zCrossState_ == NEGATIVE) {
            zCrossState_ = POSITIVE;
            zCrossArray[zCrossArrayCounter++] = index;
        }

        else if (summedToMonoSample < 0 && zCrossState_ == POSITIVE) {
            zCrossState_ = NEGATIVE;
            zCrossArray[zCrossArrayCounter++] = index;
        }
    }
}

void sand_stretchAudioProcessor::clearBuffer() {
    for (int channel = 0; channel < 2; ++channel) {
        for (int index = 0; index < maxSamplesInBuffer_; ++index) {
            bufferArray[channel][index] = 0.0f;
        }
        channelSample[channel].sampleIn = 0;
        channelSample[channel].sampleOffset = 0;
        channelSample[channel].sampleOut = 0;
    }

    for (int index = 0; index < zCrossArray.size(); ++index) {
        zCrossArray[index] = 0;
    }

    stop_ = false;
    holding_ = false;
    zCrossState_ = NONE;

}

void sand_stretchAudioProcessor::setSamples() {
    samplesSize_ = samplesParameter->load();

    if (holding_) {

        ratioSamples_ = 0;

        if (!zCrossing_) {
            samplesBoundary_ = samplesSize_;
        }

        return;
    }

    samplesBoundary_ = samplesSize_;

    ratioSamples_ = lround(samplesSize_ / ratio_);
}

void sand_stretchAudioProcessor::setBufferSize() {
    bufferSize_ = (int)bufferSizeParameter->load();
    maxSamplesInBuffer_ = sampleRate_ * bufferSize_;
    
    for (int channel = 0; channel < numInputChannels_; ++channel) {
        bufferArray[channel].resize(maxSamplesInBuffer_);
    }

    //dividing by two because input's samples cannot cross zero more times than the sample rate.
    zCrossArray.resize(maxSamplesInBuffer_/2);
}

void sand_stretchAudioProcessor::setRatio() {
    ratio_ = ratioParameter->load();

    if (holding_) {
        ratioSamples_ = 0;
        return;
    }

    ratioSamples_ = lround(samplesSize_ / ratio_);

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
    genericEditor->setSize(genericEditor->getWidth(), genericEditor->getHeight()+50);
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
