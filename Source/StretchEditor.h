#pragma once

#include "PluginEditor.h"
#include "StretchComponents.h"

namespace stretch {

	const juce::String STRETCH_VER = "1.0.0";

	using IRec = juce::Rectangle<int>;

	namespace colours {

		using Colour = juce::Colour;
		using ARGB = juce::uint32;

		const Colour background(Colour(ARGB(0xff181818))); // #181818
		const Colour component_background(Colour(ARGB(0xff303030))); // #303030
		const Colour thumb(Colour(ARGB(0xffd0d0d0))); // #d0d0d0
		const Colour slider_track(Colour(ARGB(0xffbfbfbf))); // #bfbfbf
		const Colour invisible(Colour(ARGB(0x0)));
		const Colour text(Colour(ARGB(0xfff0f0f0))); // #f0f0f0

	}

	class URLTimer : public juce::Timer {
	public:

		URLTimer(juce::Component& x) : component{ x } {}

		void timerCallback() override {
			component.setVisible(true);
		}

	private:
		juce::Component& component;
	};

	const IRec help_bounds({ (int)size_width - 35, 10, 25, 25 });
	const IRec trigger_bounds({ 37, 10	   , 75, 25 });
	const IRec hold_bounds	 ({ 37 + 75    , 10, 55, 25 });
	const IRec offset_bounds ({ 112 + 55   , 10, 50, 25 });
	const IRec tempo_toggle_bounds  ({ 167 + 50   , 10, 100, 25 });
	const IRec reverse_bounds({ 25 + 75 * 2, 10, 75, 25 });
	const IRec grain_bounds  ({ 25, 45, (int)(size_width / 1.25), 50 });
	const IRec tempo_bounds  ({ 25, 45, (int)(size_width / 1.25), 50 });
	const IRec ratio_bounds  ({ 25, 45 + 50, (int)(size_width / 1.25), 50 });

	//remember to update params header too
	const std::vector<IRec> components_bounds{
												help_bounds,
												trigger_bounds,
												hold_bounds,
												offset_bounds,
												tempo_toggle_bounds,
												grain_bounds,
												tempo_bounds,
												ratio_bounds

	};

	struct StretchBounds {
		StretchBounds() : bounds() {}
		StretchBounds(IRec x) : bounds(x) {}

		// Add xy to both x and y
		StretchBounds(IRec x, int xy)
			: bounds(x) {
			bounds.setX(bounds.getX() + xy); bounds.setY(bounds.getY() + xy);
		};

		IRec get() { return bounds; }
		IRec set(IRec x) { bounds = x; }

		IRec bounds;
	};

	//call set_font after using
	const juce::Font monospace_font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 0, juce::Font::plain));

	const String trigger_text	("Trigger:		 guess what it does");
	const String hold_text		("Hold:			 holds current grain in place");
	const String offset_text		("Offset:			 offsets grain's position when hold is on");
	const String reverse_text		("Reverse:			 outputs samples in reverse");
	const String grain_text		("Grain Size:	 how many samples to keep in one grain");
	const String ratio_text		("Ratio:		 stretches input by that amount");
	const String zcross_text	("ZCross Window: holds grains where beginning and end sample crossed 0");

	const juce::String contact_text("If you have any issues, please contact me on \ndiscord: .sandr0");
	const juce::String version_text("Ver: " + STRETCH_VER);
	const juce::URL    my_site("Placeholder");

}