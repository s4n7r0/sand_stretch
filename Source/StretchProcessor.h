#pragma once

//Things to improve upon from first version of sand_stretch:
/*
Remove buffersize and instead delete previously used samples which - DONE
can't be reached anymore. - DONE
Smooth up the sliders so it doesn't click.
Design good and fast declicking and crossfading algorithms.
	This means, 

	(declicking) DONE   find a way to smooth up the end of current grain 
						and the beginning of the next grain so they're smooth and connected
						It would be nice doing a divide and conquer thing but idk if u need grain sizes divisible by 2

    (Crossfading) done  find a way to crossfade with the next grain.
						crossfade should cover the area from
						the start of the grain up to 25%,
						and from 75% of the grain up to the end.
	- It would be useful to have the grain array be twice the size of a grain so we can store the next grain already
	- but would require adjusting or smth idk.
	+ Instead it would be better to look a bit into the future somehow, and if crossfade is at 25%
	+ and ratio would make it difficult to crossfade, clamp samples so it still gets crossfaded.
GUI in the style of napalm. - done
Don't use pointers to the buffer, and instead hold the current grain in an array. - DONE
Add a tempo option, grain size gets replaced by tempo measure - DONE

optionally:
Make sure it sounds the same as the old version. - HELL NAW
Make it sound like og dblue_stretch

*/

#include "StretchParams.h"

namespace stretch {

	const float smooth_target = 0.01;

	struct GrainInfo {
		float size;
		float ratio;

		bool using_tempo;
		float beat_duration; //in samples
		float beat_fraction;
		float beat_ratio;

		bool using_hold;
		float hold_offset;

		bool reverse;

		int declick_window;

		juce::Array<int> zcross_samples; //indexes where samples crossed 0
		int zcross_window_size;
		int zcross_window_offset;
		bool zcross_found;

		float crossfade;

		int buffer_size;
	};

	class Grain {
	public:
		Grain() : grain_offset{ 0.f }, grain_index{ 0.f }, declick_count{ 0 }, declick_sum{ 0.f } {}

		void insert_sample(const GrainInfo&, float, juce::Array<juce::String>&);
		float get_next_sample(const GrainInfo&, juce::Array<juce::String>&);

		float declick(const GrainInfo&, juce::Array<juce::String>&);
		float crossfade(const GrainInfo&, juce::Array <juce::String>&);
		void set_zcross_bounds(const GrainInfo&, juce::Array<juce::String>&);

		void clear_grain();
		void resize(int);

		void send_debug_msg(const juce::String&, juce::Array<juce::String>&);

		float grain_offset;
		float grain_index;

		bool i_found_a_zcross;

		juce::Array<float> grain_buffer;
	private:
		float local_grain_size;
		float local_grain_offset;
		float local_grain_ratio;

		int declick_count;
		float declick_sum;
		float declick_prev_sample;

		float crossfade_size;
		float crossfade_gain{ 0 };
		float crossfade_gain_value{ 0 };
		int crossfade_count{ 0 };

		int zcross_grain_size;
		int zcross_grain_offset;
		int local_zcross_window_size;
		int local_zcross_window_offset;
		int last_zcross_index{ 0 };
		int last_zcross_index_reverse{ 0 };
	};

	class Processor {
	public:
		Processor()
			: buffer_size{ 0 }, num_channels{ 2 }, sample_rate{ 44100.f }, 
			grain_info{ 16, 1, false, 0},
			buffer_is_dirty{ true }
		{
		}

		void fill_buffer(juce::AudioBuffer<float>&);
		void clear_buffer(int);

		void zcross(juce::AudioBuffer<float>&);

		void process(juce::AudioBuffer<float>&);
		void set_params(APVTS&, double bpm); // this might be stupid
		void setup(int);
		void smooth_reset(float);
		
		//enable show debug in editor
		void send_debug_msg(const juce::String&);
		void is_mismatched();

		int num_channels;
		float sample_rate;

		std::vector<Grain> grains;
		bool buffer_is_dirty;

		juce::Array<juce::String> debug_strings;
		float adjust_for_sample_rate{ 0 };

	private:
		GrainInfo grain_info;
		juce::LinearSmoothedValue<float> smooth_hold_offset;

		int buffer_size;
		bool mismatched{ false };
		ZCROSS_STATE cur_zcross_state{ NONE };

	};

}