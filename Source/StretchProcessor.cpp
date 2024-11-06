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
    
    zcross(input_buffer);

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

void Processor::zcross(juce::AudioBuffer<float>& input_buffer) {

    int num_samples = input_buffer.getNumSamples();
    int num_channels = input_buffer.getNumChannels();
    //static int zcross_index = 0;

    float summed_sample = 0;

    for (int sample = 0; sample < num_samples; ++sample) {

        for (int channel = 0; channel < num_channels; ++channel) {
            auto channelData = input_buffer.getReadPointer(channel);
            summed_sample += channelData[sample];
        }

        summed_sample /= num_channels;

        if (cur_zcross_state == ZCROSS_STATE::NONE) {
            grain_info.zcross_found = false;
            if (summed_sample > 0) cur_zcross_state = ZCROSS_STATE::ABOVE;
            if (summed_sample < 0) cur_zcross_state = ZCROSS_STATE::BELOW;
        }

        if (summed_sample > 0 && cur_zcross_state == ZCROSS_STATE::BELOW) {
            grain_info.zcross_samples.add(grain_info.buffer_size + sample);
            cur_zcross_state = ZCROSS_STATE::ABOVE;
            //send_debug_msg(String().formatted("ayy we got a zcross index at: %d", grain_info.buffer_size + sample));
        }        
        
        if (summed_sample < 0 && cur_zcross_state == ZCROSS_STATE::ABOVE) {
            grain_info.zcross_samples.add(grain_info.buffer_size + sample);
            cur_zcross_state = ZCROSS_STATE::BELOW;
            //send_debug_msg(String().formatted("ayy we got a zcross index at: %d", grain_info.buffer_size + sample));
        }

    }
}

void Processor::clear_buffer(int num_channels) {

    for (auto channel = 0; channel < num_channels; ++channel) {
        grains[channel].clear_grain();
    }

    mismatched = false;

    grain_info.zcross_samples.clear();
    cur_zcross_state = ZCROSS_STATE::NONE;

    grain_info.buffer_size = 0;
    buffer_is_dirty = false;
}

void Processor::process(juce::AudioBuffer<float>& output_buffer)
{
    //we will get enough samples either way.
    //if (grain_info.buffer_size < grain_info.size) return;

    for (auto channel = 0; channel < output_buffer.getNumChannels(); ++channel)
    {
        auto output = output_buffer.getWritePointer(channel);

        for (int sample = 0; sample < output_buffer.getNumSamples(); ++sample) {
            output[sample] = grains[channel].get_next_sample(grain_info, debug_strings);
        }

    }

    is_mismatched();

}

void Processor::set_params(APVTS& apvts, double bpm)
{

    grain_info.using_hold = (bool)apvts.getRawParameterValue("hold")->load();
    grain_info.hold_offset = apvts.getRawParameterValue("offset")->load();
    grain_info.using_tempo = (bool)apvts.getRawParameterValue("tempo_toggle")->load();

    grain_info.size = apvts.getRawParameterValue("grain")->load();
    grain_info.ratio = apvts.getRawParameterValue("ratio")->load();
    
    int beat_subdivision = (int)apvts.getRawParameterValue("subd")->load();
    grain_info.beat_duration = sample_rate * (60 / bpm); //60 s
    grain_info.beat_fraction = apvts.getRawParameterValue("tempo")->load();
    grain_info.beat_fraction = grain_info.beat_duration / std::pow(2, MAX_TEMPO_SIZE - grain_info.beat_fraction);
    grain_info.beat_fraction *= 4;

    if (grain_info.using_hold) {
        if (grain_info.using_tempo) {
            if (beat_subdivision == 1) { //if triplets
                grain_info.beat_fraction *= 1.33333f; //should be accurate enough
            }
            else if (beat_subdivision == 2) { //if dotted 
                grain_info.beat_fraction *= 1.5f;
            }
        }
    }

    grain_info.beat_ratio = grain_info.beat_fraction / grain_info.ratio;

    float temp_zcross_window_size = apvts.getRawParameterValue("zwindow")->load();
    float temp_zcross_window_offset = apvts.getRawParameterValue("zoffset")->load();
    
    bool has_zcross_window_moved = (temp_zcross_window_size != grain_info.zcross_window_size);
    bool has_zcross_offset_moved = (temp_zcross_window_offset != grain_info.zcross_window_offset);

    //this is kinda bruteforcing
    if (has_zcross_offset_moved || has_zcross_window_moved) {
        for (int channel = 0; channel < num_channels; ++channel) {
            grains[channel].i_found_a_zcross = false;
        }
    }

    grain_info.zcross_window_size = temp_zcross_window_size;
    grain_info.zcross_window_offset = temp_zcross_window_offset;

    //only crossfade up to 25% of samples
    grain_info.crossfade = apvts.getRawParameterValue("crossfade")->load();
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
    grain_offset = 0;
    grain_index = 0;

    i_found_a_zcross = false;

    declick_count = 0;

    local_grain_size = 0;
    local_grain_ratio = 0;
    local_grain_offset = 0;

    last_zcross_index = 0;

    grain_buffer.clear();
}

float Grain::get_next_sample(const GrainInfo& grain, Array<String>& dbg) 
{

    local_grain_size = grain.size;
    local_grain_offset = grain_offset;
    local_grain_ratio = grain.size / grain.ratio;
    float crossfade_percentage = grain.crossfade / 250; // up to 40%, 50% is bugged :P

    if (grain.zcross_window_size > 0 && !grain.using_tempo) {
        if (grain.using_hold) {
            set_zcross_bounds(grain, dbg);
            local_grain_size = zcross_grain_size;
            local_grain_offset = zcross_grain_offset;
        }
        else {
            //grains arent using the zcross indexes anymore, update the bounds
            i_found_a_zcross = false;
        }
    }

    if (grain.using_tempo) {
        local_grain_size = grain.beat_fraction;
        local_grain_ratio = grain.beat_fraction / grain.ratio;
    }

    //if user moved samples too fast
    if (!grain.using_hold && grain_index > local_grain_size) {
        grain_offset += grain_index - local_grain_size;
    }

    //adjust grain size depending on ratio
    if (grain.using_hold) {
        if (!grain.using_tempo && grain.zcross_window_size == 0) {
            local_grain_size -= local_grain_ratio;
        }

        //woah slow down, lemme get some samples first
        if(grain.buffer_size >= local_grain_size + MAX_HOLD_OFFSET)
            local_grain_offset += grain.hold_offset;
    }

    if (grain.crossfade >= 1) {
        crossfade_size = local_grain_size * crossfade_percentage;
        crossfade_gain_value = (1 / crossfade_size) / 2;
    }

    float sample = grain_buffer[local_grain_offset + grain_index];

    bool index_in_declick_range = (grain_index > local_grain_size - DECLICK_WINDOW / 2 || grain_index < DECLICK_WINDOW / 2);
    //figure out a way to check if ratio is greater than some size and make it work below 2 ratio
    bool grain_in_declick_range = (local_grain_ratio <= local_grain_size - DECLICK_WINDOW / 2);

    //figure this shit out
    bool index_in_crossfade_range = (local_grain_ratio <= local_grain_size - crossfade_size || true);
    bool grain_in_crossfade_range = (grain_index >= local_grain_size - crossfade_size || grain_index < crossfade_size);

    bool being_zcrossed = (!grain.using_tempo && grain.using_hold && grain.zcross_window_size > 0);

    bool index_too_big = (grain_index > local_grain_size);
    //make sure there are enough samples to declick

    //if using tempo, moving zcross window size makes samples not being declicked
    if (grain.crossfade < 1 && !being_zcrossed && !index_too_big && grain_in_declick_range && grain_offset >= DECLICK_WINDOW * 2 && index_in_declick_range) {
        sample = declick(grain, dbg);
    }

    if (grain.crossfade > 1 && !being_zcrossed && grain_in_crossfade_range && grain_offset >= crossfade_size && index_in_crossfade_range) {

        //woaah calm down, dont use that big of a gain
        //this might introduce a click
        //this could be avoided by dividing crossfade_gain by new crossfade_gain_value or smth but this is far easier
        if (crossfade_gain > 1.05) {
            crossfade_gain = 0;
        }

        //if index is reaching it's size, decrease it's volume
        float gain_in = 1 - crossfade_gain;
        float gain_out = crossfade_gain;

        if (grain_index < crossfade_size) {
            gain_in = crossfade_gain;
            gain_out = 1 - crossfade_gain;
        }

        sample = sample * gain_in + crossfade(grain, dbg) * gain_out;
    }
    else {
        crossfade_gain = 0;
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
    const int grain_offset_plus_size = (int)local_grain_offset + (int)local_grain_size;
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

        declick_prev_sample = grain_buffer[grain_offset_plus_size - DECLICK_WINDOW];

        for (int i = -DECLICK_WINDOW + declick_count; i < 0; ++i) {
            declick_sum += grain_buffer[grain_offset_plus_size + i];
        }
    } 
    else if (grain.using_hold) {
        declick_sum -= declick_prev_sample;
        declick_prev_sample = grain_buffer[grain_offset_plus_size + declick_offset];
        declick_sum += grain_buffer[(int)local_grain_offset + declick_count - 1];
    } 
    else {
        declick_sum -= declick_prev_sample;
        declick_prev_sample = grain_buffer[grain_offset_plus_size - penalty + declick_offset];
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

float Grain::crossfade(const GrainInfo& grain, juce::Array<juce::String>& dbg) {

    const int grain_offset_plus_size = (int)local_grain_offset + (int)local_grain_size;
    float crossfade_offset_hold = crossfade_size - crossfade_count;
    float crossfade_offset = -crossfade_size + crossfade_count;
    float penalty = 0;
    float penalty1 = 0;
    float sample = 0;
    float up_or_down = 1;

    if (grain_index < crossfade_size) {
        up_or_down *= -1;
        if (grain.using_hold) {
            crossfade_offset_hold -= crossfade_size;
            crossfade_offset *= up_or_down;
        }
        else {
            crossfade_offset += crossfade_size;
        }
        penalty = local_grain_ratio * 2;
        penalty1 = local_grain_size;
    }

    //if new offsets, that means we have to crossfade from previous grain 
    //if (grain_index <= crossfade_size) {
    //    up_or_down *= -1;
    //}

    if (grain.using_hold) {
        sample = grain_buffer[local_grain_offset + penalty1 + crossfade_offset_hold];
    }
    else {
        sample = grain_buffer[local_grain_offset + local_grain_ratio - penalty + penalty1 + crossfade_offset];
    }

    crossfade_count++;
    crossfade_gain += crossfade_gain_value;

    if (grain_index >= local_grain_size-1) {
        crossfade_count = 0;
    }

    return sample;
}

void  Grain::set_zcross_bounds(const GrainInfo& grain, juce::Array<juce::String>& dbg) {

    if (i_found_a_zcross) return;

    if (grain.zcross_window_size + grain.zcross_window_offset > grain.zcross_samples.size()) return;

    for (int i = last_zcross_index; i < grain.zcross_samples.size(); ++i) {

        if (grain.zcross_samples[i] >= grain_offset + grain.zcross_samples[0]) {

            //i might use it for something one day...
            int temp_zcross_offset = grain.zcross_samples[grain.zcross_window_offset + i];
            int temp_zcross_size = temp_zcross_offset - grain.zcross_samples[grain.zcross_window_offset + i - grain.zcross_window_size];

            zcross_grain_size = temp_zcross_size;
            zcross_grain_offset = grain.zcross_samples[i];
            send_debug_msg(String().formatted("zcrossoff: %d, zcrosssize: %d", zcross_grain_offset, zcross_grain_size), dbg);

            local_zcross_window_offset = grain.zcross_window_offset;
            local_zcross_window_size = grain.zcross_window_size;

            i_found_a_zcross = true;
            last_zcross_index = i; //save the index so its faster to find the next one in case offsets moved

            return;
        }
    }
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

