#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

//thank you Beats Basteln
//copied from napalm, kinda lazy
namespace stretch
{

	const float size_width = 400;
	const float size_height = 200;

	const int MAX_GRAIN_SIZE = 4096 * 4 + 1; //adding one so it doesn't click when range is set to max
	const int MAX_SAMPLES_IN_BUFFER = MAX_GRAIN_SIZE * 32; // (2 << 16) + 4
	const int MIN_GRAIN_SIZE = 16;

	const float MAX_RATIO = 12.f;
	const float MIN_RATIO = 1.f;

	const float MAX_HOLD_OFFSET = 4096; //maybe add + 1 later on

	const float MAX_ZCROSS_WINDOW_SIZE = 64;
	const int MAX_ZCROSS_HOLD_OFFSET = 1024;

	const juce::Range<float> bool_range({ 0, 1 });
	const juce::Range<float> grain_range({ MIN_GRAIN_SIZE, MAX_GRAIN_SIZE});
	const juce::Range<float> ratio_range({ MIN_RATIO, MAX_RATIO }); //lol
	//help, trigger, hold, grain, ratio
	const std::vector<juce::Range<float>> range_vector {bool_range, bool_range, bool_range, grain_range, ratio_range};

	using APVTS = juce::AudioProcessorValueTreeState;
	using Layout = APVTS::ParameterLayout;
	using P = APVTS::Parameter;
	using UniqueP = std::unique_ptr<P>;
	using UniquePVector = std::vector<UniqueP>;
	using NRange = juce::NormalisableRange<float>;
	using Attributes = juce::AudioProcessorValueTreeStateParameterAttributes;
	using paramID = juce::ParameterID;

	enum PARAMS_IDS : int { help, trigger, hold, grain, ratio, end }; //also components ids
	const std::vector<juce::String> PARAMS_STRING_IDS{ "help", "trigger", "hold", "grain", "ratio", "end"};

	inline void add_params(UniquePVector& params) {

		//this is stupid, idk of any better way
		const auto string_from_val_0d = [](float value, int max_length) { return juce::String(value, 0); };
		const auto string_from_val_2d = [](float value, int max_length) { return juce::String(value, 2); };
		const auto string_from_val_4d = [](float value, int max_length) { return juce::String(value, 4); };

		const auto val_from_string = [](juce::String value) {
			return std::stod(value.toStdString());
			};

		params.push_back(
			std::make_unique<APVTS::Parameter>(paramID{ "trigger", trigger }, "Trigger",
			NRange{ bool_range.getStart(), bool_range.getEnd(), 1.f}, 0.f,
			Attributes()
			.withStringFromValueFunction(string_from_val_0d)
			.withValueFromStringFunction(val_from_string)
			.withBoolean(true))
		);

		params.push_back(
			std::make_unique<APVTS::Parameter>(paramID{ "hold", hold }, "Hold",
			NRange{ bool_range.getStart(), bool_range.getEnd(), 1.f }, 0.f,
			Attributes()
			.withStringFromValueFunction(string_from_val_0d)
			.withValueFromStringFunction(val_from_string)
			.withBoolean(true))
		);

		params.push_back(
			std::make_unique<APVTS::Parameter>(paramID{ "grain", grain }, "Grain size", 
			NRange{ grain_range, 1.f}, 16.f,
			Attributes()
			.withStringFromValueFunction(string_from_val_0d)
			.withValueFromStringFunction(val_from_string))
		);

		params.push_back(
			std::make_unique<APVTS::Parameter>(paramID{ "ratio", ratio }, "Ratio", 
			NRange{ ratio_range, 0.01f }, 1.f,
			Attributes()
			.withStringFromValueFunction(string_from_val_2d)
			.withValueFromStringFunction(val_from_string))
		);
	}

	inline Layout create_layout() {
		UniquePVector params;

		add_params(params);

		return { params.begin(), params.end() };
	}
}