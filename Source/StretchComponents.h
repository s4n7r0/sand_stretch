/*
  ==============================================================================

    StretchComponents.h
    Created: 30 Oct 2024 9:02:08pm
    Author:  MSI

  ==============================================================================
*/
#pragma once
#include "PluginEditor.h"
namespace stretch {

	namespace components
	{

		struct AttachedComponent {

			AttachedComponent() : comp() {}

			virtual Component* get() { return &this->comp; };

			virtual void set_bounds(juce::Rectangle<int> input) { original_bounds = input; };

			juce::Rectangle<int> get_bounds() { return original_bounds; }
			juce::Rectangle<int> original_bounds;

			juce::Component comp;
		};

		struct AttachedTextButton : AttachedComponent {


			AttachedTextButton() : param{ juce::TextButton() } {}
			AttachedTextButton(juce::String text) : param{ juce::TextButton(text) } {}

			Component* get() override { return &this->param; }
			void set_bounds(juce::Rectangle<int> input) override { original_bounds = input; param.setBounds(input); };

			juce::TextButton param;
		};

		struct AttachedSlider : AttachedComponent {

			AttachedSlider(StretchAudioProcessor& p, String paramid)
				: param(), attachment(*p.apvts.getParameter(paramid), param, &p.undo)
			{
				param.setComponentID(paramid);
			}

			Component* get() override { return &this->param; }
			void set_bounds(juce::Rectangle<int> input) override { original_bounds = input; param.setBounds(input); };

			juce::Slider param;
			juce::SliderParameterAttachment attachment;
		};

		struct AttachedToggleButton : AttachedComponent {

			AttachedToggleButton(StretchAudioProcessor& p, String paramid)
				: param(paramid), attachment(*p.apvts.getParameter(paramid), param, &p.undo)
			{
				param.setComponentID(paramid);
				param.setButtonText(paramid);
			}

			Component* get() override { return &this->param; }
			void set_bounds(juce::Rectangle<int> input) override { original_bounds = input; param.setBounds(input); };

			juce::ToggleButton param;
			juce::ButtonParameterAttachment attachment;

		};
	}
}