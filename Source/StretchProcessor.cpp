#include "StretchProcessor.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"

// PROCESSOR
using namespace stretch;
void Processor::setup(int num_channels) 
{
    grains.resize(num_channels);
    buffer.resize(num_channels);

    for (auto channel = 0; channel < num_channels; ++channel) {
        grains[channel].resize(MAX_GRAIN_SIZE);
    }

    debug_strings.resize(5);
    //this->clear_buffer();
}

void Processor::fill_buffer(juce::AudioBuffer<float>& input_buffer) 
{

    int num_samples = input_buffer.getNumSamples();
    int num_channels = input_buffer.getNumChannels();

    buffer_is_dirty = true;
    grain_info.buffer_size += num_samples;
    grain_info.samples_size = num_samples;
    
    for (int channel = 0; channel < num_channels; ++channel) {

        auto channelData = input_buffer.getReadPointer(channel);

        for (int sample = 0; sample < num_samples; ++sample) {
            buffer[channel].add(channelData[sample]);
        }
    }


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
        buffer[channel].clear();
        grains[channel].clear_grain(false);
        grains[channel].resize(grain_info.size);
        
    }

    mismatched = false;

    grain_info.buffer_size = 0;
    buffer_is_dirty = false;
}

void Processor::process(juce::AudioBuffer<float>& output_buffer)
{
    //not enough samples? do nothin
    if (grain_info.buffer_size < grain_info.size) return;

    //for (auto channel = 0; channel < output_buffer.getNumChannels(); ++channel)
    //{
    auto* left = output_buffer.getWritePointer(0);
    auto* right = output_buffer.getWritePointer(1);
    for (int sample = 0; sample < output_buffer.getNumSamples(); ++sample) {
        left[sample] = grains[0].get_next_sample(grain_info, buffer[0], debug_strings);
        right[sample] = grains[1].get_next_sample(grain_info, buffer[1], debug_strings);
        is_mismatched();
        //int grains_diff = grains[0].buffer_pos - grain_info.buffer_size;
        //send_debug_msg(String().formatted("gbp: %d bs: %d diff: %d", grains[0].buffer_pos, grain_info.buffer_size, grains_diff));
    }

    //send_debug_msg(String().formatted("bufpos and bufsize diff: %d", grains[0].buffer_pos - grain_info.buffer_size));

    //}

}

void Processor::set_params(APVTS& apvts)
{

    grain_info.size = apvts.getRawParameterValue("grain")->load();
    grain_info.ratio = apvts.getRawParameterValue("ratio")->load();
    grain_info.size_ratio = std::roundf(grain_info.size / grain_info.ratio);
    grain_info.overhead = jlimit<int>(0, MAX_GRAIN_SIZE - grain_info.samples_size, grain_info.size - grain_info.samples_size);

    grain_info.holding = (bool)apvts.getRawParameterValue("hold")->load();

}

void Processor::send_debug_msg(const String& msg)
{
    debug_strings.insert(0, msg);
    debug_strings.resize(5);
}

void Processor::is_mismatched()
{
    int grains_diff = grains[0].offset_pos - grains[1].offset_pos;

    if (grains_diff && !mismatched) {
        send_debug_msg(String().formatted("desync, g[0]bufpos: %d, g[1]bufpos: %d", grains[0].offset_pos, grains[1].offset_pos));
        send_debug_msg(String().formatted("grainpos diff [0] - [1]: %d", grains[0].grain_pos - grains[1].grain_pos));
        mismatched = true;
    }
}

// GRAIN

void Grain::insert_grain(const GrainInfo& grain, Array<float>& buffer, Array<String>& dbg)
{

    do_i_need_a_grain = false;

    int size = grain.size;
    //should be handled by get_next_sample
    //int buf_diff = jlimit<int>(0, MAX_GRAIN_SIZE - grain.samples_size, size_ratio - grain.samples_size);
    //if ((grain.buffer_size - buffer_pos) < grain.size+1) {
    //    size = grain.buffer_size - buffer_pos;
    //    send_debug_msg(String().formatted("Heyy that's too big!: gs: %.2f, bs-bp: %d", grain_size_ratio, grain.buffer_size - buffer_pos), dbg);
    //    send_debug_msg(String().formatted("New size!: %d", size), dbg);

    //}
    this->clear_grain();

    for (int i = 0; i < grain.samples_size + grain.overhead; ++i) {
        cur_grain.add(*(buffer.getRawDataPointer() + grain.buffer_size - offset_pos + i));
    }
}

void Grain::clear_grain(bool quick) 
{
    if (quick) {
        cur_grain.clearQuick();
    }
    else { //we clearing it all
        offset_pos = 0;
        cur_grain.clear();
    }
    grain_pos = 0;
}

float Grain::get_next_sample(GrainInfo& grain, Array<float>& buffer, Array<String>& dbg) 
{

    //int limit = jlimit<int>(16, 256, grain.size_ratio);

    grain_size = grain.size;
    grain_ratio = grain.ratio;
    grain_size_ratio = grain.size_ratio;

    if ((int)std::roundf(grain_size_ratio) > grain.samples_size) {
        //send_debug_msg(String().formatted("Heyy that's too big!: gsr: %.2f, ss: %.2f", grain_size_ratio, grain.samples_size), dbg);
        //send_debug_msg(String().formatted("I need this much to compensate: %.2f", grain_size_ratio - grain.samples_size), dbg);
    }

    //if (do_i_need_a_grain) insert_grain(grain, buffer);
    if (grain_pos > grain_size) {
        //send_debug_msg(String().formatted("I need this much to compensate: %.2f", grain_pos - grain_size), dbg);
        grain_pos -= grain_pos - grain_size;
    }

    if (grain_pos >= grain_size) {
        if (grain_size < grain.samples_size) offset_pos -= grain_size;
        offset_pos = grain.samples_size + grain.overhead;

        insert_grain(grain, buffer, dbg);
    }

    return cur_grain[grain_pos++];

}

void Grain::resize(int new_size) {
    cur_grain.resize(new_size);
}

void Grain::send_debug_msg(const String& msg, juce::Array<juce::String>& dbg)
{
    dbg.insert(0, msg);
    dbg.resize(5);
}

