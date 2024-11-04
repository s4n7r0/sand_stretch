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

        buffer[channel].addArray(channelData[channel], num_samples);

        for (int sample = 0; sample < num_samples; ++sample) {
            grains[channel].insert_sample(grain_info, buffer[channel][sample], debug_strings);

            int samples_read = grains[channel].samples_read;

            if (grains[channel].cur_grain.size() >= MAX_GRAIN_SIZE) {
                grains[channel].samples_read = 0;
                grains[channel].cur_grain.removeRange(0, num_samples);
            }
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
        grains[channel].buffer_offset = 0;
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

    int sample = 0;

    //subtract newly added samples
    for (int channel = 0; channel < num_channels; ++channel) {
        grains[channel].grain_index -= grain_info.samples_size;
    }

    for (sample = 0; sample < grain_info.samples_size; ++sample) {
        left[sample] = grains[0].get_next_sample(grain_info, sample, debug_strings);
        right[sample] = grains[1].get_next_sample(grain_info, sample, debug_strings);
        is_mismatched();
        //int grains_diff = grains[0].buffer_pos - grain_info.buffer_size;
        //send_debug_msg(String().formatted("gbp: %d bs: %d diff: %d", grains[0].buffer_pos, grain_info.buffer_size, grains_diff));
    }

    //store where we left off
    for (int channel = 0; channel < num_channels; ++channel) {
        grains[channel].grain_offset += sample;
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

    int max_samples = grain.samples_size + grain.overhead;

    //int buf_diff = jlimit<int>(0, MAX_GRAIN_SIZE - grain.samples_size, grain_offset - grain.samples_size);

    for (int i = 0; i < max_samples; ++i) {
        samples_read += 1;
        float sample = *(buffer.getRawDataPointer() + (grain.buffer_size - max_samples) + i);

        cur_grain.add(sample);
    }
}

void Grain::insert_sample(const GrainInfo& grain, float sample, juce::Array<juce::String>& dbg)
{
    cur_grain.add(sample);
    samples_read += 1;
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
}

float Grain::get_next_sample(GrainInfo& grain, float index, Array<String>& dbg) 
{

    //int limit = jlimit<int>(16, 256, grain.size_ratio);
    int size_ratio_diff = grain.size - grain.size_ratio;

    grain_size = grain.size;
    grain_ratio = grain.ratio;
    grain_size_ratio = grain.size_ratio;
    grain_index += 1;

    if ((int)std::roundf(grain_size_ratio) > grain.samples_size) {
        //send_debug_msg(String().formatted("Heyy that's too big!: gsr: %.2f, ss: %.2f", grain_size_ratio, grain.samples_size), dbg);
        //send_debug_msg(String().formatted("I need this much to compensate: %.2f", grain_size_ratio - grain.samples_size), dbg);
    }

    //if (grain_pos > grain_size) {
    //    //send_debug_msg(String().formatted("I need this much to compensate: %.2f", grain_pos - grain_size), dbg);
    //    grain_pos -= grain_pos - grain_size;
    //}
    //int buf_diff = jlimit<int>(0, MAX_GRAIN_SIZE - grain.samples_size, grain_offset - grain.samples_size);

    //if (grain_offset + grain_pos >= grain.samples_size + grain.overhead) {
    //    grain_offset = 0;
    //    //insert_grain(grain, buffer, dbg);
    //    do_i_need_a_grain = true;
    //    //return get_next_sample(grain, buffer, dbg);
    //}

    if (grain_index >= grain_size_ratio)
        grain_index -= size_ratio_diff;

    //int sample = -grain.size + grain_index;

    //index is process's for loop ( -grain_info.size + sample) where
    return cur_grain[cur_grain.size() + grain_index];

}

void Grain::resize(int new_size) {
    cur_grain.resize(new_size);
}

void Grain::send_debug_msg(const String& msg, juce::Array<juce::String>& dbg)
{
    dbg.insert(0, msg);
    dbg.resize(5);
}

