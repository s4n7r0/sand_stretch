#include "StretchProcessor.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"

// PROCESSOR
using namespace stretch;
void Processor::setup() 
{
    buffer.resize(num_channels);
    grains.resize(num_channels);

    for (auto channel = 0; channel < num_channels; ++channel) {
        grains[channel].resize(MAX_GRAIN_SIZE);
    }

    debug_strings.resize(5);
    //this->clear_buffer();
}

void Processor::fill_buffer(juce::AudioBuffer<float>& input_buffer) 
{

    int num_samples = input_buffer.getNumSamples();

    for (auto channel = 0; channel < num_channels; ++channel) {
        buffer[channel].insertArray(-1, input_buffer.getReadPointer(channel), num_samples);
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

void Processor::clear_buffer() {

    for (auto channel = 0; channel < num_channels; ++channel) {
        buffer[channel].clear();
        grains[channel].clear_grain(false);
        grains[channel].resize(grain_info.size);
    }

    grain_info.buffer_size = 0;
    //grain_info.buffer_pos = 0;
    buffer_is_dirty = false;
}

void Processor::process(juce::AudioBuffer<float>& output_buffer)
{
    // cmon do something with the ratio...
    //if ((buffer_size - grain_info.buffer_pos) < grain_info.size) return;

    for (auto channel = 0; channel < num_channels; ++channel)
    {
        auto* channelData = output_buffer.getWritePointer(channel);

        for (int sample = 0; sample < output_buffer.getNumSamples(); ++sample) {
            channelData[sample] = grains[channel].get_next_sample(grain_info, buffer[channel]);
        }

    }

    // we done, lets move
    //grain_info.buffer_pos += grain_info.size_ratio;
}

void Processor::set_params(APVTS& apvts)
{
    grain_info.size = apvts.getRawParameterValue("grain")->load();
    grain_info.ratio = apvts.getRawParameterValue("ratio")->load();
    grain_info.size_ratio = std::roundf(grain_info.size / grain_info.ratio);

    send_debug_msg(String().formatted("%.5d", grain_info.ratio));

    grain_info.holding = (bool)apvts.getRawParameterValue("hold")->load();

    DBG("Woah new sizes!\n" << 
        "grain: " << grain_info.size << 
        "\n ratio: " << grain_info.ratio << 
        "\n size_ratio: " << grain_info.size_ratio << "\n");
}

void Processor::send_debug_msg(String& msg) {
    debug_strings.insert(0, msg);
    debug_strings.resize(5);
}

// GRAIN

void Grain::insert_grain(const GrainInfo& grain, Array<float>& buffer)
{

    do_i_need_a_grain = false;
    float size = grain.size;
    //grain.buffer_size - buffer_pos;

    //if (grain.size_ratio > grain.samples_size) {
    //    size = grain.samples_size;
    //};
    int test = buffer_pos - (int)grain.size;
    int temp = jlimit<int>(0, MAX_GRAIN_SIZE, test);
    //jlimit<int>()

    this->clear_grain();
    //this->resize(size);
    cur_grain.insertArray(0, buffer.getRawDataPointer() + buffer_pos - (int)grain.size_ratio, size);
}

void Grain::clear_grain(bool quick) 
{
    if (quick) {
        cur_grain.clearQuick();
    }
    else { //we clearing it all
        buffer_pos = 0;
        cur_grain.clear();
    }
    grain_pos = 0;
}

float Grain::get_next_sample(GrainInfo& grain, Array<float>& buffer) 
{

    if (do_i_need_a_grain) insert_grain(grain, buffer);

    if (grain_pos >= grain.size) {
        if (!buffer_pos) buffer_pos += grain.size_ratio - 1;
        else buffer_pos += grain.size_ratio;

        do_i_need_a_grain = true;
        return get_next_sample(grain, buffer);
    }

    return cur_grain[grain_pos++];

}

void Grain::resize(int new_size) {
    cur_grain.resize(new_size);
}
