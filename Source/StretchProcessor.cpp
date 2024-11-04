#include "StretchProcessor.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"

// PROCESSOR
using namespace stretch;
void Processor::setup(int num_channels) 
{
    grains.resize(num_channels);

    debug_strings.resize(5);

}

void Processor::fill_buffer(juce::AudioBuffer<float>& input_buffer) 
{

    int num_samples = input_buffer.getNumSamples();
    int num_channels = input_buffer.getNumChannels();
    
    for (int channel = 0; channel < num_channels; ++channel) {

        auto channelData = input_buffer.getReadPointer(channel);

        for (int sample = 0; sample < num_samples; ++sample) {
            grains[channel].insert_sample(grain_info, channelData[sample], debug_strings);

            int samples_read = grains[channel].samples_read;
        }
    }

    buffer_is_dirty = true;
    grain_info.buffer_size += num_samples;
    grain_info.samples_size = num_samples;

    //clear buffer based on how far current grain is into the buffer
    //since we cant go back

    //if (buffer_pos >= MAX_SAMPLES_IN_BUFFER * 2) {
    //	for (auto channel = 0; channel < input_buffer.getNumChannels(); ++channel) {
    //		buffer[channel].removeRange(0, MAX_SAMPLES_IN_BUFFER);
    //		buffer_pos = buffer[channel].size();
    //	}
    //}
}

void Processor::clear_buffer(int num_channels) {

    for (auto channel = 0; channel < num_channels; ++channel) {
        grains[channel].clear_grain();
        grains[channel].buffer_offset = 0;
        grains[channel].grain_index = 0;
        grains[channel].grain_offset = 0;
        grains[channel].samples_read = 0;
    }

    mismatched = false;

    grain_info.buffer_size = 0;
    buffer_is_dirty = false;
}

void Processor::process(juce::AudioBuffer<float>& output_buffer)
{
    //not enough samples? do nothin
    if (grain_info.buffer_size < grain_info.size) return;

    for (auto channel = 0; channel < output_buffer.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < grain_info.samples_size; ++sample) {
            auto output = output_buffer.getWritePointer(channel);
            output[sample] = grains[channel].get_next_sample(grain_info, sample, debug_strings);
        }
    }
    is_mismatched();
}

void Processor::set_params(APVTS& apvts)
{

    grain_info.size = apvts.getRawParameterValue("grain")->load();
    grain_info.ratio = apvts.getRawParameterValue("ratio")->load();
    grain_info.size_ratio = std::roundf(grain_info.size / grain_info.ratio);

    for (int channel = 0; channel < num_channels; ++channel) {
        if (grains[channel].grain_offset < 0) 
            grains[channel].grain_offset -= grains[channel].grain_offset;
    }

    grain_info.holding = (bool)apvts.getRawParameterValue("hold")->load();

}

void Processor::send_debug_msg(const String& msg)
{
    debug_strings.insert(0, msg);
    debug_strings.resize(5);
}

void Processor::is_mismatched()
{
    int grains_diff = grains[0].grain_offset - grains[1].grain_offset;

    if (grains_diff && !mismatched) {
        send_debug_msg(String().formatted("desync, g[0]offset: %d, g[1]offset: %d", grains[0].grain_offset, grains[1].grain_offset));
        send_debug_msg(String().formatted("offset diff [0] - [1]: %d", grains[0].grain_offset - grains[1].grain_offset));
        mismatched = true;
    }
}

// GRAIN

void Grain::insert_sample(const GrainInfo& grain, float sample, juce::Array<juce::String>& dbg)
{
    cur_grain.add(sample);
    samples_read += 1;
}

void Grain::clear_grain() 
{
    cur_grain.resize(0);
    cur_grain.clear();
}

float Grain::get_next_sample(GrainInfo& grain, float index, Array<String>& dbg) 
{

    if (grain_index >= grain.size) {
        grain_offset += grain.size_ratio;
        grain_index = 0;
    }

    if (grain_offset < 0) {
        grain_offset += -grain_offset;
        if (grain_offset < 0)
            send_debug_msg(String().formatted("I need this much to compensate: %.2f", -grain_offset ), dbg);
    }

    return cur_grain[grain_index++ + grain_offset];

}

void Grain::resize(int new_size) {
    cur_grain.resize(new_size);
}

void Grain::send_debug_msg(const String& msg, juce::Array<juce::String>& dbg)
{
    dbg.insert(0, msg);
    dbg.resize(5);
}

