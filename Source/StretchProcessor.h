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
GUI in the style of napalm.
Don't use pointers to the buffer, and instead hold the current grain in an array.

optionally:
Make sure it sounds the same as the old version.

*/

#include "StretchParams.h"

namespace stretch {

	struct GrainInfo {
		float size;
		float ratio;
		float size_ratio;
		int pos;
	};

	class Grain {
	public:
		Grain() : sample_pos{ 0.f }, limit_reached{ false }, cur_grain{} {}

		void insert_grain(const GrainInfo&, juce::Array<float>&);
		float get_next_sample(GrainInfo&);
		//void set_grain(GrainInfo&, float);
		bool do_i_need_a_grain();
		// 0 - prev, 1 - current, 2 - next
		juce::Array<float> get_grain(GrainInfo&, int);
		void clear_grain();
		void resize(int);

	private:
		// keep 25% of prev and next grain so we can easily crossfade
		juce::Array<float> cur_grain;
		
		float sample_pos;
		bool limit_reached;
	};

	class Processor {
	public:
		Processor() 
			: sample_rate{ 44100.f }, buffer_size{ 0 }, 
			  num_channels{ 0 }, grain_info{ 0,0,0,0 },
			  buffer_is_dirty{false}
		{
		};

		void fill_buffer(juce::AudioBuffer<float>&);
		void clear_buffer();

		void set_grain(APVTS&); // this might be stupid
		void setup();
		void process(juce::AudioBuffer<float>&);
		
		float sample_rate;
		int num_channels;
		std::vector<Grain> grains;
		bool buffer_is_dirty;

	private:
		GrainInfo grain_info;
		std::vector<juce::Array<float>> buffer;
		int buffer_size;
	};

}