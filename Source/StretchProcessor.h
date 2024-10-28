#pragma once
#include <JuceHeader.h>
namespace stretch {

	using APVTS = juce::AudioProcessorValueTreeState;

	struct channelStruct {
		int in = 0;
		float out = 0;
		float offset = 0;
	};

	struct channelDeclickStruct {
		bool is_declicked = false;
		bool declicking = false;
	};

	struct channelCrossfadeStruct {
		float increasing = 0;
		float decreasing = 1;
		int in = 0;
		int out = 0;
	};

	enum enumZCROSSSTATE { NEGATIVE, NONE, POSITIVE };

	class StretchProcessor {
	public:
		StretchProcessor()
		{
		};

		void process(juce::AudioBuffer<float>&, APVTS&);
		void set_buffer_size(int);
		void set_ratio(float);
		void set_declick(int);
		void set_samples(int);
		void clear_buffer();
		void insert_to_buffer(float, int, int);
		void setup_arrays();

		double sample_rate;
		int num_input_channels = 0;
		
	private:

		int buffer_size{ 0 };
		int max_samples_in_buffer = 0;

		int samples_size = 256;
		int samples_boundary = 0;

		int ratio_samples = 1;

		float ratio = 1.0f;

		bool stop = false;
		bool cleared = false;
		bool holding = false;
		bool zcrossing = false;
		bool ratio_changed = false;

		enumZCROSSSTATE zcross_state = NONE;
		int zcross_window = 0;
		int zcross_offset = 0;
		int zcross_hold_offset = 0;
		bool zcross_hold_offset_moved = false;

		bool crossfading = false;
		std::vector<channelCrossfadeStruct> crossfade_channel;

		std::vector<std::vector<float>> buffer_array;
		//TODO: Dynamically adjust zCrossArray to fit all samples
		std::vector<float> zcross_array;
		std::vector<channelStruct> channel_sample;
		std::vector<float> previous_sample_offsets;

		std::array<int, 6> declick_choices = { 0, 32, 64, 128, 256, 512 };
		int declick_window = 64;
		int half_window = declick_window >> 1;
		int quarter_window = half_window >> 1;
		int declick_window_minus_one = declick_window - 1;
		int half_window_minus_one = half_window - 1;
		const int declick_quality = 1 << 0; //4
		std::vector<std::array<float, 512>> declick_samples;
		std::vector<channelDeclickStruct> declick_channel;
		int declick_choice = 2;

	};
}