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
     : stretch_processor(getSampleRate(), getNumInputChannels()), AudioProcessor(BusesProperties()
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
    sampleRate_ = sampleRate;
    numInputChannels_ = getTotalNumInputChannels();
    channelSample.resize(numInputChannels_);
    previousSampleOffsets.resize(numInputChannels_);
    crossfadeChannel.resize(numInputChannels_);
    declickSamples.resize(numInputChannels_);
    declickChannel.resize(numInputChannels_);
    bufferArray.resize(numInputChannels_);

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

    stretch_processor.process(buffer, parameters);

    auto trigger = triggerParameter->load();
    auto hold = holdParameter->load();
    auto reverse = reverseParameter->load();
    auto removeDcOffset = removeDcOffsetParameter->load();
    auto declick = declickParameter->load();
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

    if (declickWindow != declick) {
        stretch_processor.set_declick((int)declickParameter->load());
        setDeclickWindow((int)declickParameter->load());
    }

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel) {

        bool dcOffsetFound = false;
        float dcOffsetSample{};

        auto sampleIn = buffer.getReadPointer(channel);
        auto sampleOut = buffer.getWritePointer(channel);

        if (!trigger) {
            for (auto channel : channelSample) {
                channel.sampleIn = 0;
                channel.sampleOffset = 0;
                channel.sampleOut = 0;
            }
        }

        int& currentSampleIn = channelSample[channel].sampleIn;
        float& currentSampleOut = channelSample[channel].sampleOut;
        float& currentSampleOffset = channelSample[channel].sampleOffset;

        if (trigger && hold && !holding_) {
            holding_ = true;

            stretch_processor.set_ratio(ratio);
            setRatio();
        }

        if (!hold && holding_) {
            holding_ = false;

            if (zCrossing_) {
                stretch_processor.set_samples(samplesSize);
                setSamples();
            }

            zCrossing_ = false;
            zCrossHoldOffsetMoved_ = true;

            stretch_processor.set_ratio(ratio);
            setRatio();
        }

        if (trigger) {

            cleared_ = false;

            for (auto sample = 0; sample < buffer.getNumSamples(); ++sample) {

                if (stop_) break;

                stretch_processor.insert_to_buffer(sampleIn[sample], currentSampleIn, channel);
                insertToBuffer(sampleIn[sample], currentSampleIn, channel);
                currentSampleIn++;

                if (currentSampleIn >= maxSamplesInBuffer_) break;
            }

            if (currentSampleIn <= samplesSize_) continue;

            for (auto sample = 0; sample < buffer.getNumSamples(); ++sample) {

                //zCross
                //channel needs to be 0 otherwise it may get desynced when hold and zcrossing was on before the trigger.
                if (holding_ && zCrossWindow > 0 && channel == 0 && (zCrossWindow != zCrossWindow_ || zCrossOffset != zCrossOffset_)) {
                    
                    zCrossWindow_ = zCrossWindow;
                    zCrossOffset_ = zCrossOffset;
                    
                    int offset = 0;
                    int windowAndOffset = zCrossWindow_ + zCrossOffset_;

                    //makes sure there are atleast 3 samples in the window, maybe useless idk.
                    while ((zCrossArray[windowAndOffset + offset] - zCrossArray[windowAndOffset - zCrossWindow_]) <= 3 && windowAndOffset != 0) {
                        windowAndOffset--;
                        zCrossWindow_--;
                        //if (zCrossArray[windowAndOffset + offset] <= 1) break;
                    }

                    //get offset of where currently held sample is.
                    if (zCrossHoldOffsetMoved_) {
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

                int holding = holding_;

                int currentOutAndOffsetForwards = currentSampleOut + currentSampleOffset;
                int currentOutAndOffsetBackwards = currentSampleOffset + samplesBoundary_ - ((samplesSize_ / ratio) * holding) * skipSamples - currentSampleOut - 1;
                //subtracting by one so it doesn't go past the boundary.

                //this may desync the audio a bit.
                //if hold was on before the trigger, this may result in a negative index.
                if (currentOutAndOffsetBackwards < 0) {
                    for (int channel = 0; channel < numInputChannels_; ++channel) {
                        channelSample[channel].sampleOffset += 2;
                    }
                    //currentSampleOut = 0;
                    //sample--; //this is dangerous, but prevents desync in some cases.
                    continue;
                }

                int forwardsIndex = currentOutAndOffsetForwards + holdOffset * holding;
                int backwardsIndex = currentOutAndOffsetBackwards + holdOffset * holding;

                if (forwardsIndex >= maxSamplesInBuffer_ || backwardsIndex >= maxSamplesInBuffer_) {
                    continue;
                }

                //reset hold to where it was before zcrossing
                if (holding_ && zCrossing_ && zCrossWindow == 0) {
                    zCrossing_ = false;

                    for (int channel = 0; channel < numInputChannels_; ++channel) {
                        channelSample[channel].sampleOffset = previousSampleOffsets[channel];
                        previousSampleOffsets[channel] = 0.f;
                    }

                    stretch_processor.set_samples(samplesSize);
                    setSamples();
                }

                float outputForwards = bufferArray[channel][forwardsIndex];
                float outputBackwards = bufferArray[channel][backwardsIndex];

                if (reverse) {
                    std::swap(outputForwards, outputBackwards);
                    std::swap(forwardsIndex, backwardsIndex);
                }     
                
                float boundaryWithOffsetAndShit = (samplesBoundary_ * skipSamples) + currentSampleOffset + holdOffset;

                //DC Offset
                if (removeDcOffset && !dcOffsetFound && samplesBoundary_ > 0) {

                    float average{};

                    for (float i = currentSampleOffset + holdOffset; i < boundaryWithOffsetAndShit; i += 1 * skipSamples) {
                        if (i >= maxSamplesInBuffer_) break;
                        average += bufferArray[channel][i];
                    }

                    dcOffsetSample = std::clamp((average / (samplesBoundary_ * skipSamples )) * -1, -1.f, 1.f);

                    dcOffsetFound = true;
                }

                sampleOut[sample] = outputForwards + dcOffsetSample;

                int zCrossing = !zCrossing_;

                //Crossfade
                if (crossfade >= 0.01f) {
                    float actualBoundary = (samplesBoundary_ * skipSamples - ((samplesSize_ / ratio) * holding) * zCrossing);
                    int crossfadeSamples = actualBoundary * crossfade;
                    int halfCrossfadeSamples = crossfadeSamples / 2;
                    int localHoldOffset = holdOffset * holding;
                    int curSampleOffsetAndHoldOffset = currentSampleOffset + localHoldOffset;

                    //clamp if offset is too small for crossfading
                    if (curSampleOffsetAndHoldOffset - halfCrossfadeSamples < 0) {
                        crossfadeSamples = curSampleOffsetAndHoldOffset;
                        halfCrossfadeSamples = crossfadeSamples / 2;
                    }

                    if (curSampleOffsetAndHoldOffset - ratioSamples_ - halfCrossfadeSamples < 0) { //may happen when hold is switched on and off quickly.
                        crossfadeSamples = curSampleOffsetAndHoldOffset - ratioSamples_;
                        halfCrossfadeSamples = crossfadeSamples / 2; 
                    }

                    int indexFadingIn = curSampleOffsetAndHoldOffset + ratioSamples_ - halfCrossfadeSamples;
                    int indexFadingOut = curSampleOffsetAndHoldOffset - ratioSamples_ - halfCrossfadeSamples;

                    float& increasing = crossfadeChannel[channel].increasing;
                    float& decreasing = crossfadeChannel[channel].decreasing;
                    int& indexIn = crossfadeChannel[channel].indexIn;
                    int& indexOut = crossfadeChannel[channel].indexOut;

                    float crossfadeMorph = 1 / (float)crossfadeSamples;

                    if (actualBoundary - halfCrossfadeSamples <= 0) {
                        crossfadeSamples = actualBoundary;
                        halfCrossfadeSamples = crossfadeSamples / 2;
                    }

                    if (currentSampleOut >= actualBoundary - halfCrossfadeSamples && !(actualBoundary <= crossfadeSamples)) {
                        crossfading = true;
                        sampleOut[sample] = bufferArray[channel][forwardsIndex] * decreasing + bufferArray[channel][indexFadingIn + indexIn] * increasing;
                        sampleOut[sample] += dcOffsetSample;

                        decreasing = std::clamp(decreasing - crossfadeMorph, 0.f, 1.f); //sometimes when samples or ratio are changed these can go below 0 or above 1
                        increasing = std::clamp(increasing + crossfadeMorph, 0.f, 1.f);
                        indexIn++;
                    }
                    else if (currentSampleOut < halfCrossfadeSamples) {
                        sampleOut[sample] = bufferArray[channel][indexFadingOut + indexOut] * decreasing + bufferArray[channel][forwardsIndex] * increasing;
                        sampleOut[sample] += dcOffsetSample;

                        decreasing = std::clamp(decreasing - crossfadeMorph, 0.f, 1.f);
                        increasing = std::clamp(increasing + crossfadeMorph, 0.f, 1.f);
                        indexOut++;
                    }
                    else {
                        crossfading = false;
                        decreasing = 1;
                        increasing = 0;
                        indexIn = indexOut = 0;
                    }
                }

                //Declick
                if (declick && samplesBoundary_ > declickWindow) {

                    const std::vector<float>::const_iterator buffer = bufferArray[channel].begin() + currentSampleOffset;

                    float actualBoundary = (samplesBoundary_ * skipSamples - ((samplesSize_ / ratio) * holding) * zCrossing);
                    if (actualBoundary <= declickWindow) {

                        stretch_processor.set_declick((int)declickParameter->load() - 1);

                        setDeclickWindow((int)declickParameter->load()-1);
                    }

                    int localHoldOffset = holdOffset * holding;

                    bool &areSamplesDeclicked = declickChannel[channel].areSamplesDeclicked;
                    bool &declicking = declickChannel[channel].declicking;

                    if (!areSamplesDeclicked && !declicking) {

                        //the "click" should be in the middle of the array
                        std::copy_n(buffer + localHoldOffset + actualBoundary - halfWindowMinusOne, halfWindow, declickSamples[channel].begin());
                        std::copy_n(buffer + localHoldOffset + ratioSamples_, halfWindow, declickSamples[channel].begin() + halfWindow);

                        if (reverse) std::reverse(declickSamples[channel].begin(), declickSamples[channel].begin() + declickWindow);

                        float sum{};
                        float sum2{};
                        for (int i = 0; i < halfWindow; ++i) {
                            sum += declickSamples[channel].at(i);
                            sum2 += declickSamples[channel].at(declickWindowMinusOne - i);
                        }

                        float morphRatio = 2 / (float)halfWindow; 
                        float decreasing = 1;
                        float increasing = 0;
                        for (int i = 1; i < quarterWindow; ++i) {

                            int iTimesTwo = i << 1;

                            //only use every second element, sum - even, sum2 - odd
                            sum = (sum - declickSamples[channel].at(i)) + declickSamples[channel].at(halfWindow + iTimesTwo);
                            sum2 = (sum2 - declickSamples[channel].at(declickWindowMinusOne - iTimesTwo)) + declickSamples[channel].at(halfWindowMinusOne - iTimesTwo);

                            //normalise samples to -1 <-> 1 range.
                            float mean = sum / declickWindow;
                            float mean2 = sum2 / declickWindow;

                            //float decreasing = (halfWindow - iTimesTwo) / (float)halfWindow;
                            //float increasing = (iTimesTwo / (float)halfWindow);
                            decreasing -= morphRatio; //avoid divison -> bit faster.
                            increasing += morphRatio;

                            //morph original samples into declicked ones.
                            declickSamples[channel].at(iTimesTwo) = declickSamples[channel].at(i) * decreasing + mean * increasing;
                            declickSamples[channel].at(iTimesTwo - 1) = (declickSamples[channel].at(iTimesTwo - 2) + declickSamples[channel].at(iTimesTwo)) / 2; //fill a sample in between

                            declickSamples[channel].at(declickWindowMinusOne - iTimesTwo) = declickSamples[channel].at(declickWindowMinusOne - iTimesTwo) * decreasing + mean2 * increasing;
                            declickSamples[channel].at(declickWindow - iTimesTwo) = (declickSamples[channel].at(declickWindowMinusOne - iTimesTwo) + declickSamples[channel].at(declickWindow + 1 - iTimesTwo)) / 2;

                            //if declickWindow is even, treat last two samples seperately
                            //                      checking last bit to see if even 
                            if (i == quarterWindow - 1 && !(declickWindow & 1)) {
                                float temp = declickSamples[channel].at(halfWindowMinusOne);
                                declickSamples[channel].at(halfWindowMinusOne) = (declickSamples[channel].at(halfWindow-2) + declickSamples[channel].at(halfWindow + 1) + declickSamples[channel].at(halfWindow + 2)) / 3;
                                declickSamples[channel].at(halfWindow) = (declickSamples[channel].at(halfWindow + 2) + declickSamples[channel].at(halfWindow + 3)  + declickSamples[channel].at(halfWindow - 2)) / 3;
                            }
                        }
                        areSamplesDeclicked = true;
                    }

                    int index = halfWindow - (actualBoundary - currentSampleOut);

                    if (currentSampleOut >= actualBoundary - halfWindow  && !(index >= declickWindow || index < 0)) {
                        declicking = true;
                        sampleOut[sample] = declickSamples[channel].at(index);
                        sampleOut[sample] += dcOffsetSample;
                    }
                    else if (currentSampleOut < halfWindow) {
                        sampleOut[sample] = declickSamples[channel].at(halfWindow + currentSampleOut);
                        sampleOut[sample] += dcOffsetSample;
                    }
                    else {
                        declicking = false;
                        areSamplesDeclicked = false;
                    }
                }

                currentSampleOut += 1 * skipSamples;

                if (currentSampleOut >= samplesBoundary_ * skipSamples - ((samplesSize_ / ratio) * holding) * zCrossing) {
                    currentSampleOffset += ratioSamples_;
                    currentSampleOut = 0;
                }
            }
        }
        
        if (!trigger && !cleared_) {

            stretch_processor.clear_buffer();

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
    for (int channel = 0; channel < numInputChannels_; ++channel) {
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
    for(auto channel : declickChannel) channel.areSamplesDeclicked = false;

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
    for (auto channel : declickChannel) channel.areSamplesDeclicked = false;

    if (holding_) {
        ratioSamples_ = 0;
        return;
    }

    ratioSamples_ = lround(samplesSize_ / ratio_);
}

void sand_stretchAudioProcessor::setDeclickWindow(int windowIndex = 0) {
    int newWindow = declickChoices[windowIndex];
    if (!newWindow) return;

    declickWindow = newWindow;
    halfWindow = declickWindow >> 1;
    quarterWindow = halfWindow >> 1;
    declickWindowMinusOne = declickWindow - 1;
    halfWindowMinusOne = halfWindow - 1;
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
