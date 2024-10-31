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
	const IRec trigger_bounds({ 25, 10, 75, 25 });
	const IRec hold_bounds({ 100, 10, 75, 25 });
	//const IRec hold_offset_bounds({ (int)(size_width / 1.175), 85, 50, 50 }); use the semitone thing from napalm to set offset instead of another slider
	const IRec grain_bounds({ 25, 35, (int)(size_width / 1.25), 50 });
	const IRec ratio_bounds({ 25, 85, (int)(size_width / 1.25), 50 });
	//const IRec zcross_window_bounds({ 25, 85, (int)(size_width / 1.25), 50 });
	//const IRec zcross_offset_bounds({ (int)(size_width / 1.175), 85, 50, 50 }); use the semitone thing from napalm to set offset instead of another slider

	const std::vector<IRec> components_bounds{
												help_bounds,
												trigger_bounds,
												hold_bounds,
												grain_bounds,
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

	const String trigger_text	("Trigger:		 blablabla");
	const String hold_text		("Hold:			 blablabla");
	const String grain_text		("Grain Size:	 blablabla");
	const String ratio_text		("Ratio:		 blablabla");
	const String zcross_text	("ZCross Window: blablabla");

	const juce::String contact_text("If you have any issues, please contact me on \ndiscord: .sandr0");
	const juce::String version_text("Ver: " + STRETCH_VER);
	const juce::URL    my_site("Placeholder");

}