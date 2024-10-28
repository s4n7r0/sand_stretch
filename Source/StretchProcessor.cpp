#include "StretchProcessor.h"

using namespace stretch;

void StretchProcessor::process(juce::AudioBuffer<float>& buffer, APVTS& apvts) {
    auto p_trigger = apvts.getRawParameterValue("triggerParameter")->load();
    auto p_hold = apvts.getRawParameterValue("holdParameter")->load();
    auto p_reverse = apvts.getRawParameterValue("reverseParameter")->load();
    auto p_remove_dc_offset = apvts.getRawParameterValue("removeDcOffsetParameter")->load();
    auto p_declick = apvts.getRawParameterValue("declickParameter")->load();
    auto p_ratio = apvts.getRawParameterValue("ratioParameter")->load();
    auto p_samples_size = apvts.getRawParameterValue("samplesParameter")->load();
    auto p_skip_samples = apvts.getRawParameterValue("skipSamplesParameter")->load();
    auto p_hold_offset = apvts.getRawParameterValue("holdOffsetParameter")->load();
    auto p_crossfade = apvts.getRawParameterValue("crossfadeParameter")->load() / 200.0f; //dividing by 200 to get values in range 0-0.5
    auto p_zcross_window = apvts.getRawParameterValue("zcrossParameter")->load();
    auto p_zcross_offset = apvts.getRawParameterValue("zcrossOffsetParameter")->load();
    auto p_buffer_size = apvts.getRawParameterValue("bufferSizeParameter")->load();

    check_params(p_samples_size, p_ratio, p_buffer_size, p_declick);

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel) {

        bool dc_offset_found = false;
        float dc_offset_sample{};

        auto sample_in = buffer.getReadPointer(channel);
        auto sample_out = buffer.getWritePointer(channel);

        if (!p_trigger) {
            for (auto channel : channel_sample) {
                channel.in = 0;
                channel.offset = 0;
                channel.out = 0;
            }
        }

        int& current_sample_in = channel_sample[channel].in;
        float& current_sample_out = channel_sample[channel].out;
        float& current_sample_offset = channel_sample[channel].offset;

        if (p_trigger && p_hold && !holding) {
            holding = true;

            set_ratio(p_ratio);
        }

        if (!p_hold && holding) {
            holding = false;

            if (zcrossing) {
                set_samples(p_samples_size);
            }

            zcrossing = false;
            zcross_hold_offset_moved = true;

            set_ratio(ratio);
        }

        if (p_trigger) {

            cleared = false;

            for (auto sample = 0; sample < buffer.getNumSamples(); ++sample) {

                if (stop) break;

                insert_to_buffer(sample_in[sample], current_sample_in, channel);
                current_sample_in++;

                if (current_sample_in >= max_samples_in_buffer) break;

            }

            if (current_sample_in <= samples_size) continue;

            for (auto sample = 0; sample < buffer.getNumSamples(); ++sample) {

                //zCross
                //channel needs to be 0 otherwise it may get desynced when hold and zcrossing was on before the trigger.
                /*
                if (holding && p_zcross_window > 0 && channel == 0 && (zcross_window != p_zcross_window || zcross_offset != p_zcross_offset)) {

                    zcross_window = p_zcross_window;
                    zcross_offset = p_zcross_offset;

                    int offset = 0;
                    int window_and_offset = zcross_window + zcross_offset;

                    //makes sure there are atleast 3 samples in the window, maybe useless idk.
                    while ((zcross_array[window_and_offset + offset] - zcross_array[window_and_offset - p_zcross_window]) <= 3 && window_and_offset != 0) {
                        window_and_offset--;
                        zcross_window--;
                        //if (zcross_array[window_and_offset+ offset] <= 1) break;
                    }

                    //get offset of where currently held sample is.
                    if (zcross_hold_offset_moved) {
                        for (int i = 0; i < zcross_array.size(); ++i) {
                            //sometimes it hangs up but resumes some time later
                            //probably something to do with floats
                            if (zcross_array[i] <= 0) break;
                            if (zcross_array[i] >= current_sample_offset + zcross_array[0]) {
                                zcross_hold_offset = i;
                                zcross_hold_offset_moved = false;
                                break;
                            }
                        }
                    }

                    window_and_offset += zcross_hold_offset;

                    samples_boundary = zcross_array[window_and_offset + offset] - zcross_array[window_and_offset - p_zcross_window];

                    if (!zcrossing) {
                        for (int channel = 0; channel < num_input_channels; ++channel) {
                            previous_sample_offsets[channel] = channel_sample[channel].offset;
                        }
                        zcrossing = true;
                    }

                    for (int channel = 0; channel < num_input_channels; ++channel) {
                        channel_sample[channel].offset = zcross_array[window_and_offset];
                    }
                }
                */

                int temp_holding = holding;

                int currentOutAndOffsetForwards = current_sample_out + current_sample_offset;
                int currentOutAndOffsetBackwards = current_sample_offset + samples_boundary - ((samples_size / ratio) * holding) * p_skip_samples - current_sample_out - 1;
                //subtracting by one so it doesn't go past the boundary.

                //this may desync the audio a bit.
                //if hold was on before the trigger, this may result in a negative index.
                if (currentOutAndOffsetBackwards < 0) {
                    for (int channel = 0; channel < num_input_channels; ++channel) {
                        channel_sample[channel].offset += 2;
                    }
                    //current_sample_out = 0;
                    //sample--; //this is dangerous, but prevents desync in some cases.
                    continue;
                }

                int forwardsIndex = currentOutAndOffsetForwards + p_hold_offset * holding;
                int backwardsIndex = currentOutAndOffsetBackwards + p_hold_offset * holding;

                if (forwardsIndex >= max_samples_in_buffer || backwardsIndex >= max_samples_in_buffer) {
                    continue;
                }

                //reset hold to where it was before zcrossing
                if (holding && zcrossing && zcross_window == 0) {
                    zcrossing = false;

                    for (int channel = 0; channel < num_input_channels; ++channel) {
                        channel_sample[channel].offset = previous_sample_offsets[channel];
                        previous_sample_offsets[channel] = 0.f;
                    }

                    set_samples(p_samples_size);
                }
                

                float outputForwards = buffer_array[channel][forwardsIndex];
                float outputBackwards = buffer_array[channel][backwardsIndex];

                if (p_reverse) {
                    std::swap(outputForwards, outputBackwards);
                    std::swap(forwardsIndex, backwardsIndex);
                }

                float boundaryWithOffsetAndShit = (samples_boundary * p_skip_samples) + current_sample_offset + p_hold_offset;

                //DC Offset
                /*
                if (p_remove_dc_offset && !dc_offset_found && samples_boundary > 0) {

                    float average{};

                    for (float i = current_sample_offset + p_hold_offset; i < boundaryWithOffsetAndShit; i += 1 * p_skip_samples) {
                        if (i >= max_samples_in_buffer) break;
                        average += buffer_array[channel][i];
                    }

                    dc_offset_sample = std::clamp((average / (samples_boundary * p_skip_samples)) * -1, -1.f, 1.f);

                    dc_offset_found = true;
                }
                */

                sample_out[sample] = outputForwards + dc_offset_sample;

                int zCrossing = !zcrossing;

                //Crossfade
                /*
                if (p_crossfade >= 0.01f) {
                    float actualBoundary = (samples_boundary * p_skip_samples - ((samples_size / ratio) * holding) * zCrossing);
                    int crossfadeSamples = actualBoundary * p_crossfade;
                    int halfCrossfadeSamples = crossfadeSamples / 2;
                    int local_hold_offset = p_hold_offset * holding;
                    int cur_sample_offset_and_hold_offset = current_sample_offset + local_hold_offset;

                    //clamp if offset is too small for crossfading
                    if (cur_sample_offset_and_hold_offset - halfCrossfadeSamples < 0) {
                        crossfadeSamples = cur_sample_offset_and_hold_offset;
                        halfCrossfadeSamples = crossfadeSamples / 2;
                    }

                    if (cur_sample_offset_and_hold_offset - ratio_samples - halfCrossfadeSamples < 0) { //may happen when hold is switched on and off quickly.
                        crossfadeSamples = cur_sample_offset_and_hold_offset - ratio_samples;
                        halfCrossfadeSamples = crossfadeSamples / 2;
                    }

                    int indexFadingIn = cur_sample_offset_and_hold_offset + ratio_samples - halfCrossfadeSamples;
                    int indexFadingOut = cur_sample_offset_and_hold_offset - ratio_samples - halfCrossfadeSamples;

                    float& increasing = crossfade_channel[channel].increasing;
                    float& decreasing = crossfade_channel[channel].decreasing;
                    int& indexIn = crossfade_channel[channel].in;
                    int& indexOut = crossfade_channel[channel].out;

                    float crossfadeMorph = 1 / (float)crossfadeSamples;

                    if (actualBoundary - halfCrossfadeSamples <= 0) {
                        crossfadeSamples = actualBoundary;
                        halfCrossfadeSamples = crossfadeSamples / 2;
                    }

                    if (current_sample_out >= actualBoundary - halfCrossfadeSamples && !(actualBoundary <= crossfadeSamples)) {
                        crossfading = true;
                        sample_out[sample] = buffer_array[channel][forwardsIndex] * decreasing + buffer_array[channel][indexFadingIn + indexIn] * increasing;
                        sample_out[sample] += dc_offset_sample;

                        decreasing = std::clamp(decreasing - crossfadeMorph, 0.f, 1.f); //sometimes when samples or ratio are changed these can go below 0 or above 1
                        increasing = std::clamp(increasing + crossfadeMorph, 0.f, 1.f);
                        indexIn++;
                    }
                    else if (current_sample_out < halfCrossfadeSamples) {
                        sample_out[sample] = buffer_array[channel][indexFadingOut + indexOut] * decreasing + buffer_array[channel][forwardsIndex] * increasing;
                        sample_out[sample] += dc_offset_sample;

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
                */

                //Declick
                /*
                if (p_remove_dc_offset && samples_boundary > declick_window) {

                    //float* temp_buffer = buffer_array[channel].getRawDataPointer() + (int)current_sample_offset;
                    juce::Array<float> temp_buffer;

                    float actualBoundary = (samples_boundary * p_skip_samples - ((samples_size / ratio) * holding) * zCrossing);
                    if (actualBoundary <= declick_window) {
                        set_declick(p_declick - 1);
                    }

                    int local_hold_offset = p_hold_offset * holding;

                    bool& areSamplesDeclicked = declick_channel[channel].is_declicked;
                    bool& declicking = declick_channel[channel].declicking;

                    if (!areSamplesDeclicked && !declicking) {

                        //the "click" should be in the middle of the array
                        //std::copy_n(temp_buffer + local_hold_offset + (int)actualBoundary - half_window_minus_one, half_window, declick_samples[channel].begin());
                        //std::copy_n(temp_buffer + local_hold_offset + ratio_samples, half_window, declick_samples[channel].begin() + half_window);
                        declick_samples[channel].insertArray(-1, buffer_array[channel].getRawDataPointer() + local_hold_offset + (int)actualBoundary - half_window, half_window);
                        declick_samples[channel].insertArray(-1, buffer_array[channel].getRawDataPointer() + local_hold_offset + ratio_samples, half_window);
                        if (p_reverse) std::reverse(declick_samples[channel].begin(), declick_samples[channel].begin() + declick_window);

                        float sum{};
                        float sum2{};
                        for (int i = 0; i < half_window; ++i) {
                            sum += declick_samples[channel][i];
                            sum2 += declick_samples[channel][declick_window_minus_one - i];
                        }

                        float morphRatio = 2 / (float)half_window;
                        float decreasing = 1;
                        float increasing = 0;
                        for (int i = 1; i < quarter_window; ++i) {

                            int iTimesTwo = i << 1;

                            //only use every second element, sum - even, sum2 - odd
                            sum = (sum - declick_samples[channel][i]) + declick_samples[channel][half_window + iTimesTwo];
                            sum2 = (sum2 - declick_samples[channel][declick_window_minus_one - iTimesTwo]) + declick_samples[channel][half_window_minus_one - iTimesTwo];

                            //normalise samples to -1 <-> 1 range.
                            float mean = sum / declick_window;
                            float mean2 = sum2 / declick_window;

                            //float decreasing = (half_window - iTimesTwo) / (float)half_window;
                            //float increasing = (iTimesTwo / (float)half_window);
                            decreasing -= morphRatio; //avoid divison -> bit faster.
                            increasing += morphRatio;

                            //morph original samples into declicked ones.
                            declick_samples[channel].set(iTimesTwo, declick_samples[channel][i] * decreasing + mean * increasing);
                            declick_samples[channel].set(iTimesTwo - 1, declick_samples[channel][iTimesTwo - 2] + declick_samples[channel][iTimesTwo] / 2); //fill a sample in between
                                                    
                            declick_samples[channel].set(declick_window_minus_one - iTimesTwo, declick_samples[channel][declick_window_minus_one - iTimesTwo] * decreasing + mean2 * increasing);
                            declick_samples[channel].set(declick_window - iTimesTwo, declick_samples[channel][declick_window_minus_one - iTimesTwo] + declick_samples[channel][declick_window + 1 - iTimesTwo] / 2);

                            //if declick_window is even, treat last two samples seperately
                            //                      checking last bit to see if even 
                            if (i == quarter_window - 1 && !(declick_window & 1)) {
                                float temp = declick_samples[channel][half_window_minus_one];
                                declick_samples[channel].set(half_window_minus_one, declick_samples[channel][half_window - 2] + declick_samples[channel][half_window + 1] + declick_samples[channel][half_window + 2] / 3);
                                declick_samples[channel].set(half_window, declick_samples[channel][half_window + 2] + declick_samples[channel][half_window + 3] + declick_samples[channel][half_window - 2] / 3);
                            }
                        }
                        areSamplesDeclicked = true;
                    }

                    int index = half_window - (actualBoundary - current_sample_out);

                    if (current_sample_out >= actualBoundary - half_window && !(index >= declick_window || index < 0)) {
                        declicking = true;
                        sample_out[sample] = declick_samples[channel][index];
                        sample_out[sample] += dc_offset_sample;
                    }
                    else if (current_sample_out < half_window) {
                        sample_out[sample] = declick_samples[channel][half_window + current_sample_out];
                        sample_out[sample] += dc_offset_sample;
                    }
                    else {
                        declicking = false;
                        areSamplesDeclicked = false;
                    }
                }
                */

                current_sample_out += 1 * p_skip_samples;

                if (current_sample_out >= samples_boundary * p_skip_samples - ((samples_size / ratio) * holding) * zCrossing) {
                    current_sample_offset += ratio_samples;
                    current_sample_out = 0;
                }
            }
        }
        if (!p_trigger && !cleared) {

            clear_buffer();

            cleared = true;
        }
    }

}

inline void StretchProcessor::check_params(int p_sample_size, float p_ratio, int p_buffer_size, int p_declick_window) {
    if (p_sample_size != samples_size) {
        set_samples(p_sample_size);
    }

    if (p_ratio != ratio) {
        set_ratio(p_ratio);
    }

    if (p_buffer_size != buffer_size) {
        set_buffer_size(p_buffer_size);
    }

    if (p_declick_window != declick_window) {
        set_declick(p_declick_window);
    }
}

void StretchProcessor::insert_to_buffer(float sample, int index, int channel) {
    //make sure samples from all channels are gathered.
    for (int i = 0; i < num_input_channels; ++i) {
        if (channel_sample[i].in >= max_samples_in_buffer) {
            if (i == num_input_channels - 1) stop = true;
            return;
        }
    }

    buffer_array[channel].add(sample);

    //collect indexes of points where summed samples crossed zero.
    static int zcross_array_counter = 0;

    if (channel == num_input_channels - 1) {

        float summed_to_mono{};

        for (int i = 0; i < num_input_channels; ++i) {
            summed_to_mono += buffer_array[i][index];
        }

        summed_to_mono /= num_input_channels;

        if (zcross_state == NONE) {
            zcross_array_counter = 0;
            if (summed_to_mono > 0) {
                zcross_state = POSITIVE;
            }
            else {
                zcross_state = NEGATIVE;
            }
        }

        if (zcross_array_counter >= zcross_array.size()) return;

        if (summed_to_mono > 0 && zcross_state == NEGATIVE) {
            zcross_state = POSITIVE;
            zcross_array[zcross_array_counter++] = index;
        }

        else if (summed_to_mono < 0 && zcross_state == POSITIVE) {
            zcross_state = NEGATIVE;
            zcross_array[zcross_array_counter++] = index;
        }
    }
}

void StretchProcessor::set_buffer_size(int new_size) {
    buffer_size = new_size;
    max_samples_in_buffer = sample_rate * buffer_size;

    for (int channel = 0; channel < num_input_channels; ++channel) {
        buffer_array[channel].resize(max_samples_in_buffer);
    }

    //std::abort();

    //dividing by two because input's samples cannot cross zero more times than the sample rate.
    zcross_array.resize(max_samples_in_buffer / 2);
}

void StretchProcessor::clear_buffer() {
    for (int channel = 0; channel < num_input_channels; ++channel) {
        for (int index = 0; index < max_samples_in_buffer; ++index) {
            buffer_array[channel].removeRange(0, max_samples_in_buffer);
        }
        channel_sample[channel].in = 0;
        channel_sample[channel].offset = 0;
        channel_sample[channel].out = 0;
    }

    for (int index = 0; index < zcross_array.size(); ++index) {
        zcross_array[index] = 0;
    }

    stop = false;
    holding = false;
    zcross_state = NONE;
    cleared = true;

}

void StretchProcessor::set_ratio(float new_ratio) {
    ratio = new_ratio;
    for (auto channel : declick_channel) channel.is_declicked = false;

    if (holding) {
        ratio_samples = 0;
        return;
    }

    ratio_samples = lround(samples_size / ratio);
}

void StretchProcessor::set_declick(int new_index = 0) {
    if (!new_index) return;
    int newWindow = declick_choices[new_index];

    declick_window = newWindow;
    half_window = declick_window >> 1;
    quarter_window = half_window >> 1;
    declick_window_minus_one = declick_window - 1;
    half_window_minus_one = half_window - 1;
}

void StretchProcessor::set_samples(int new_samples) {
    samples_size = new_samples;
    for (auto channel : declick_channel) channel.is_declicked = false;

    if (holding) {

        ratio_samples = 0;

        if (!zcrossing) {
            samples_boundary = samples_size;
        }

        return;
    }

    samples_boundary = samples_size;

    ratio_samples = lround(samples_size / ratio);
}

void StretchProcessor::setup_arrays()
{   
    channel_sample.resize(num_input_channels);
    previous_sample_offsets.resize(num_input_channels);
    crossfade_channel.resize(num_input_channels);
    declick_samples.resize(num_input_channels);
    declick_channel.resize(num_input_channels);
    buffer_array.resize(num_input_channels);

    set_buffer_size(8);
}
