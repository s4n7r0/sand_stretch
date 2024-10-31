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
	It would be useful to have the grain array be twice the size of a grain so we can store the next grain already
	but would require adjusting or smth idk.
GUI in the style of napalm.
Don't use pointers to the buffer, and instead hold the current grain in an array.

optionally:
Make sure it sounds the same as the old version.

*/

#include "StretchParams.h"

namespace stretch {

	class Grain {
	public:
		Grain() : size{ MIN_GRAIN_SIZE }, ratio{ MIN_RATIO }, current_pos{ 0 } {}
	private:
		float size;
		float ratio;

		juce::Array<float> all_grain;
		juce::Array<float> cur_grain;

		// let's keep the current pos of a grain so we can 
		// play from the right place in the next process block
		float current_pos;
	};

	class Processor {
		public:
			Processor() {}

			float sample_rate;
		private:
			Grain grains;
	};

}