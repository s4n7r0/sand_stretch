#pragma once

//Things to improve upon from first version of sand_stretch:
/*
Remove buffersize and instead delete previously used samples which 
can't be reached anymore. - DONE
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
Don't use pointers to the buffer, and instead hold the current grain in an array. - DONE
Add a tempo option, grain size gets replaced by tempo measure

optionally:
Make sure it sounds the same as the old version. - HELL NAW
Make it sound like og dblue_stretch

*/

#include "StretchParams.h"

namespace stretch {

	struct GrainInfo {
		float size;
		float ratio;
		float size_ratio; 

		bool using_tempo;
		float beat_duration; //in samples
		float beat_fraction;

		bool using_hold;
		float hold_offset;

		int buffer_size;
	};

	class Grain {
	public:
		Grain() : grain_offset{ 0 }, grain_index{ 0 } {}

		void insert_sample(const GrainInfo&, float, juce::Array<juce::String>&);
		float get_next_sample(const GrainInfo&, juce::Array<juce::String>&);

		void clear_grain();
		void resize(int);

		void send_debug_msg(const juce::String&, juce::Array<juce::String>&);

		float grain_offset;
		float grain_index;

		juce::Array<float> grain_buffer;
	private:

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
		void set_params(APVTS&, double bpm); // this might be stupid
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

		int buffer_size;
		bool mismatched{ false };

	};

}