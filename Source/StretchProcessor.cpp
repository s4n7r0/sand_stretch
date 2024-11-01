#include "StretchProcessor.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"

// PROCESSOR
using namespace stretch;
void Processor::setup() 
{
	buffer.resize(num_channels);
	grains.resize(num_channels);
}

void Processor::fill_buffer(juce::AudioBuffer<float>& input_buffer) 
{

	int num_samples = input_buffer.getNumSamples();

	for (auto channel = 0; channel < num_channels; ++channel) {
		buffer[channel].insertArray(-1, input_buffer.getReadPointer(channel), num_samples);
	}

	buffer_is_dirty = true;
	buffer_size += num_samples;

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
		grains[channel].clear_grain();
	}
	buffer_size = 0;
	buffer_is_dirty = false;
}

void Processor::set_grain(APVTS& apvts)
{
	grain_info.size = apvts.getRawParameterValue("grain")->load();
	grain_info.ratio = apvts.getRawParameterValue("ratio")->load();
	grain_info.size_ratio = grain_info.size / grain_info.ratio;
	DBG("Woah new sizes!\n" << 
		"grain: " << grain_info.size << 
		"\n ratio: " << grain_info.ratio << 
		"\n size_ratio: " << grain_info.size_ratio << "\n");

	for (auto channel = 0; channel < num_channels; ++channel) {
		grains[channel].resize(grain_info.size);
	}
}


void Processor::process(juce::AudioBuffer<float>& output_buffer)
{

	if (buffer_size < grain_info.size) {
		return;
	}

	for (auto channel = 0; channel < num_channels; ++channel) {
		if(grains[channel].do_i_need_a_grain())
			grains[channel].insert_grain(grain_info, buffer[channel]);
	}

	for (auto channel = 0; channel < num_channels; ++channel)
	{
		auto* channelData = output_buffer.getWritePointer(channel);

		for (int sample = 0; sample < output_buffer.getNumSamples(); ++sample) {

			if (!grains[channel].do_i_need_a_grain())
			{
				channelData[sample] = grains[channel].get_next_sample(grain_info);
			}
			else {
				grains[channel].insert_grain(grain_info, buffer[channel]);
			}
		}
	}

}

// GRAIN

void Grain::insert_grain(const GrainInfo& grain, juce::Array<float>& buffer)
{
	cur_grain.removeRange(0, grain.size);
	this->resize(grain.size);
	cur_grain.insertArray(grain.size - 1, buffer.getRawDataPointer() + grain.pos, grain.size);
	limit_reached = false;
	sample_pos = 0;
}

//void Grain::set_grain(float new_size, float new_ratio)
//{
//	size = new_size;
//	ratio = new_ratio;
//	size_ratio = size / ratio;
//	
//}

//change into returning samples
juce::Array<float> Grain::get_grain(GrainInfo& grain, int which = 1)
{
	juce::Array<float> temp;
	const float& size = grain.size;

	switch (which)
	{	
	case 0:
		temp.insertArray(-1, cur_grain.begin(), size);
		return temp;
	case 1:
		temp.insertArray(-1, cur_grain.begin() + std::lroundf(size) - 1, size); //std::
		cur_grain.removeRange(0, size);
		return temp;
	case 2:
		temp.insertArray(-1, cur_grain.begin() + (std::lroundf(size) * 2) - 1, size);
		return temp;
	//default:
	//	//unreachable
	//	return;
	}
}

float Grain::get_next_sample(GrainInfo& grain) {

	if (limit_reached) return 0.f;
	if (sample_pos < grain.size)
		return cur_grain[grain.size - 1 + sample_pos++];

	limit_reached = true;
	grain.pos += grain.size_ratio;
	sample_pos = 0;

}

bool Grain::do_i_need_a_grain() {
	if (cur_grain.size() <= 0) return true;
	return limit_reached;
}

void Grain::clear_grain() {
	cur_grain.clear();
	sample_pos = 0;
}

void Grain::resize(int new_size) {
	cur_grain.resize(new_size * 3);
}