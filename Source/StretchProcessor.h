#pragma once

//Things to improve upon from first version of sand_stretch:
/*
Remove buffersize and instead delete previously used samples which 
can't be reached anymore.
Smooth up the sliders so it doesn't click.
Design good and fast declicking and crossfading algorithms.
	This means, 
	(declicking)   find a way to smooth up the end of current grain 
			       and the beginning of the next grain so they're smooth and connected
				   It would be nice doing a divide and conquer thing but idk if u need grain sizes divisible by 2
    (Crossfading)  find a way to crossfade with the next grain.
				   crossfade should cover the area from
				   the start of the grain up to 25%,
				   and from 75% of the grain up to the end.
	- It would be useful to have the grain array be twice the size of a grain so we can store the next grain already
	- but would require adjusting or smth idk.
	+ Instead it would be better to look a bit into the future somehow, and if crossfade is at 25%
	+ and ratio would make it difficult to crossfade, clamp samples so it still gets crossfaded.
GUI in the style of napalm. - done
Don't use pointers to the buffer, and instead hold the current grain in an array.
Add a tempo option, grain size gets replaced by tempo measure

optionally:
Make sure it sounds the same as the old version.

*/

#include "StretchParams.h"

namespace stretch {

	struct GrainInfo {
		float size;
		float ratio;
		float size_ratio; 
		bool holding;

		int overhead; //how much samples we're over the samples in buffer

		int buffer_size;
		int samples_size;
	};

	class Grain {
	public:
		Grain() 
			: grain_pos{ 0.f }, grain_size{ 16.f }, offset_pos{ 0 },
		      do_i_need_a_grain{ true }, cur_grain{} 
		{
		}

		void insert_grain(const GrainInfo&, juce::Array<float>&, juce::Array<juce::String>&);
		void insert_sample(const GrainInfo&, float, juce::Array<juce::String>&);
		void clear_grain(bool quick = true);
		float get_next_sample(GrainInfo&, float, juce::Array<juce::String>&);

		void resize(int);
		void send_debug_msg(const juce::String&, juce::Array<juce::String>&);

		int offset_pos; //if grain size is less than buffer size
		float grain_pos{ 0.f }; //index of current grain's sample
		float grain_offset{ 0 };
		int buffer_offset{ 0 };
		//bool can_i_send_dbg_pls{ false };
		int samples_read{ 0 };
		int grain_index{ 0 };
		juce::Array<float> cur_grain;
	private:
		// keep 25% of prev and next grain so we can easily crossfade
		
		float grain_size;
		float grain_ratio;
		float grain_size_ratio;
		bool do_i_need_a_grain;

	};

	class Processor {
	public:
		Processor()
			: buffer_size{ 0 }, num_channels{ 2 }, sample_rate{ 44100.f }, 
			grain_info{ 16, 1, 16, false, 0},
			buffer_is_dirty{ true }
		{
		}

		void fill_buffer(juce::AudioBuffer<float>&);
		void clear_buffer(int);

		void process(juce::AudioBuffer<float>&);
		void set_params(APVTS&); // this might be stupid
		void setup(int);
		
		void send_debug_msg(const juce::String&);
		void is_mismatched();

		int num_channels;
		float sample_rate;

		std::vector<Grain> grains;
		bool buffer_is_dirty;

		juce::Array<juce::String> debug_strings;

	private:
		GrainInfo grain_info;
		std::vector<juce::Array<float>> buffer;
		int buffer_size;
		bool mismatched{ false };

	};

}