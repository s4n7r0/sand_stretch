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
        }

        int limit = MAX_GRAIN_SIZE * 64; // we need a lot for tempo...
        if(grain_info.using_hold)
            limit += grain_info.hold_offset;

        //this might be an issue if using_hold...
        if ((int)grains[channel].grain_offset >= limit) {
            grain_info.buffer_size -= MAX_GRAIN_SIZE;
            grains[channel].grain_buffer.removeRange(0, MAX_GRAIN_SIZE);
            grains[channel].grain_offset -= MAX_GRAIN_SIZE;
                //if(channel == num_channels - 1) 
                    //send_debug_msg(String().formatted("new size: %d", grains[channel].grain_buffer.size()));
        }

    }

    buffer_is_dirty = true;
    grain_info.buffer_size += num_samples;

}

void Processor::clear_buffer(int num_channels) {

    for (auto channel = 0; channel < num_channels; ++channel) {
        grains[channel].clear_grain();
    }

    mismatched = false;

    grain_info.buffer_size = 0;
    buffer_is_dirty = false;
}

void Processor::process(juce::AudioBuffer<float>& output_buffer)
{
    //we will get enough samples either way.
    //if (grain_info.buffer_size < grain_info.size) return;

    for (auto channel = 0; channel < output_buffer.getNumChannels()-1; ++channel)
    {
        auto outputl = output_buffer.getWritePointer(0);
        auto outputr = output_buffer.getWritePointer(1);

        for (int sample = 0; sample < output_buffer.getNumSamples(); ++sample) {
            outputl[sample] = grains[0].get_next_sample(grain_info, debug_strings);
            outputr[sample] = grains[1].get_next_sample(grain_info, debug_strings);
            is_mismatched();
        }

    }

}

void Processor::set_params(APVTS& apvts, double bpm)
{

    grain_info.using_hold = (bool)apvts.getRawParameterValue("hold")->load();
    grain_info.hold_offset = apvts.getRawParameterValue("offset")->load();
    grain_info.using_tempo = (bool)apvts.getRawParameterValue("tempo_toggle")->load();

    grain_info.size = apvts.getRawParameterValue("grain")->load();
    grain_info.ratio = apvts.getRawParameterValue("ratio")->load();
    grain_info.size_ratio = grain_info.size / grain_info.ratio;
    
    double subdivision = 1;
    int beat_subdivision = (int)apvts.getRawParameterValue("subd")->load();
    char dbg_subd_str = 'n';
    grain_info.beat_duration = sample_rate * (60 / bpm); //60 s
    grain_info.beat_fraction = apvts.getRawParameterValue("tempo")->load();
    double temp = grain_info.beat_fraction;
    grain_info.beat_fraction = grain_info.beat_duration / std::pow(2, MAX_TEMPO_SIZE - grain_info.beat_fraction);
    grain_info.beat_fraction *= 4;

    if (grain_info.using_hold) {
        if (grain_info.using_tempo) {
            if (beat_subdivision == 1) { //if triplets
                grain_info.beat_fraction *= 1.33333f; //should be accurate enough
                dbg_subd_str = 't';
            }
            else if (beat_subdivision == 2) { //if dotted 
                grain_info.beat_fraction *= 1.5f;
                dbg_subd_str = 'd';
            }
        }
    }

    //grain_info.beat_fraction = grain_info.beat_duration * (grain_info.beat_fraction / 16);
    grain_info.beat_ratio = grain_info.beat_fraction / grain_info.ratio;
    //send_debug_msg(String().formatted("1/%.0lf %c fraction in samples: %.0lf", std::pow(2, MAX_TEMPO_SIZE - temp), dbg_subd_str, grain_info.beat_fraction));
    //send_debug_msg(String().formatted("%.01f/16 fraction in samples: %.0lf", temp / 16, grain_info.beat_fraction));
    //grain_info.size_ratio = grain_info.size / grain_info.ratio;
}

void Processor::send_debug_msg(const String& msg)
{
    debug_strings.insert(0, msg);
    debug_strings.resize(5);
}

void Processor::is_mismatched()
{

    float grains_diff = grains[0].grain_offset - grains[1].grain_offset;
    float index_diff = grains[0].grain_index - grains[1].grain_index;

    if ((bool)grains_diff || (bool)index_diff && !mismatched) {
        send_debug_msg(String().formatted("desync, g[0]offset: %f, g[1]offset: %f", grains[0].grain_offset, grains[1].grain_offset));
        send_debug_msg(String().formatted("offset diff [0] - [1]: %f", grains[0].grain_offset - grains[1].grain_offset));        
        send_debug_msg(String().formatted("desync, g[0]index: %f, g[1]index: %f", grains[0].grain_index, grains[1].grain_index));
        send_debug_msg(String().formatted("index diff [0] - [1]: %d", grains[0].grain_index - grains[1].grain_index));
        mismatched = true;
    }
}

// GRAIN

void Grain::clear_grain() 
{
    grain_index = 0;
    grain_offset = 0;
    declick_count = 0;
    local_grain_size = 0;
    local_grain_ratio = 0;
    local_grain_offset = 0;
    grain_buffer.clear();
}

float Grain::get_next_sample(const GrainInfo& grain, Array<String>& dbg) 
{

    local_grain_size = grain.size;
    local_grain_offset = grain_offset;
    local_grain_ratio = grain.size_ratio;

    if (grain.using_tempo) {
        local_grain_size = grain.beat_fraction;
        local_grain_ratio = grain.beat_fraction / grain.ratio;
    }

    //if user moved samples too fast
    if (!grain.using_hold && grain_index > local_grain_size) {
        grain_offset += grain_index - local_grain_size;
        send_debug_msg(String().formatted("hehe"), dbg);

    }

    //adjust grain size depending on ratio
    if (grain.using_hold) {
        if (!grain.using_tempo) {
            local_grain_size -= local_grain_ratio;
        }

        //woah slow down, lemme get some samples first
        if(grain.buffer_size >= local_grain_size + MAX_HOLD_OFFSET)
            local_grain_offset += grain.hold_offset;
    }


    float sample = grain_buffer[local_grain_offset + grain_index];

    // * 2 to make sure we have enough samples to calculate the sum from
    bool is_index_in_declick_range = (grain_index > local_grain_size - DECLICK_WINDOW / 2 || grain_index < DECLICK_WINDOW / 2);
    //make sure there are enough samples to declick
    //figure out a way to check if ratio is greater than some size and make it work below 2 ratio
    if (local_grain_ratio <= local_grain_size - DECLICK_WINDOW / 2 && grain_offset >= DECLICK_WINDOW * 2 && is_index_in_declick_range) {
        sample = declick(grain, dbg);
    }

    grain_index++;

    //if grain reached it's limits
    if (grain_index >= local_grain_size) {

        //make sure there are enough samples in the buffer, and then
        if (!grain.using_hold && grain.buffer_size >= local_grain_size) {
            //move it further into the grain's buffer
            grain_offset += local_grain_ratio;
        }

        // using hold, so just reset the index and stay in place.
        grain_index = 0;
    }

    return sample;
}

float Grain::declick(const GrainInfo& grain, juce::Array<juce::String>& dbg) {

    //moving average

    const int declick_offset = declick_count - DECLICK_WINDOW;
    const int offset_plus_size = (int)local_grain_offset + (int)local_grain_size;
    int penalty = 0;
    float sample = 0;

    //adjust from where we read samples if not holding, since offset moved it's place
    if (grain_index <= DECLICK_WINDOW / 2) {
        penalty = local_grain_ratio;
    }

    //let's calculate samples up till the end of our current grain
    //decreasing the index we're starting from the further we are in the declicking process
    //so we're moving the window

    //looks like we just started
    if (declick_count == 0) {

        declick_prev_sample = grain_buffer[offset_plus_size - DECLICK_WINDOW];

        for (int i = -DECLICK_WINDOW + declick_count; i < 0; ++i) {
            declick_sum += grain_buffer[offset_plus_size + i];
        }
    } 
    else if (grain.using_hold) {
        declick_sum -= declick_prev_sample;
        declick_prev_sample = grain_buffer[offset_plus_size + declick_offset];
        declick_sum += grain_buffer[(int)local_grain_offset + declick_count - 1];
    } 
    else {
        declick_sum -= declick_prev_sample;
        declick_prev_sample = grain_buffer[offset_plus_size - penalty + declick_offset];
        declick_sum += grain_buffer[(int)local_grain_offset + local_grain_ratio - penalty + declick_count - 1];
    }

    sample = declick_sum / DECLICK_WINDOW;
    declick_count++;

    //if index is about to reach the end of declicking window, reset the count
    if (grain_index + 1 + DECLICK_WINDOW / 2 == DECLICK_WINDOW) {
        declick_count = 0;
        declick_sum = 0;
        declick_prev_sample = 0;
    }

    return sample;
}

void Grain::insert_sample(const GrainInfo& grain, float sample, juce::Array<juce::String>& dbg)
{
    grain_buffer.add(sample);
}

void Grain::resize(int new_size) {
    grain_buffer.resize(new_size);
}

void Grain::send_debug_msg(const String& msg, juce::Array<juce::String>& dbg)
{
    dbg.insert(0, msg);
    dbg.resize(5);
}

