/*
  ==============================================================================

    StretchEditor.cpp
    Created: 31 Oct 2024 12:20:58am
    Author:  MSI

  ==============================================================================
*/

#include "PluginEditor.h"

using namespace stretch;

void StretchAudioProcessorEditor::show_or_hide() {

	for (int i = PARAMS_IDS::trigger; i < PARAMS_IDS::end; ++i) {
		components[i]->get()->setVisible(!help_state);
	}

}

void StretchAudioProcessorEditor::set_scales() {
	x_scale = getWidth() / size_width;
	y_scale = getHeight() / size_height;
	abs_scale = std::log2f(std::fabs(((size_width - getWidth()) / size_width) * ((size_height - getHeight()) / size_height)) + 2.f);
	slider_width = getWidth() - (size_width / 5) * abs_scale;
}

void StretchAudioProcessorEditor::draw_labels(juce::Graphics& g) {

	using namespace stretch;

	IRec temp_bounds = grain_text_bounds.bounds;
	temp_bounds *= abs_scale;
	temp_bounds.setWidth(slider_width);
	g.drawFittedText(juce::String("grain"), temp_bounds, juce::Justification::right, 4, 0);

	temp_bounds = ratio_text_bounds.bounds;
	temp_bounds *= abs_scale;
	temp_bounds.setWidth(slider_width);
	g.drawFittedText(juce::String("ratio"), temp_bounds, juce::Justification::right, 4, 0);

	set_font_size(g, 15 * abs_scale);

}

void StretchAudioProcessorEditor::draw_help(juce::Graphics& g) {

	components::AttachedTextButton* help = dynamic_cast<components::AttachedTextButton*>(components[stretch::PARAMS_IDS::help]);

	if (help->param.isMouseOver()) {

		if (!help_state) help_state = true;

		g.fillAll(colours::background);
		set_font_size(g, 12 * abs_scale);

		int text_y = 30 * abs_scale;

		for (int i = 0; i < help_texts.size(); ++i) {
			g.drawFittedText(help_texts[i], juce::Rectangle<int>(25, text_y - 25 + ((text_y / 1.5) * i), getWidth(), 25), juce::Justification::left, 1, 1);
		}

		IRec contact_text_bounds = juce::Rectangle<int>(0, getHeight() - text_y * 2, getWidth(), 25);
		IRec version_text_bounds = juce::Rectangle<int>(0, getHeight() - text_y, getWidth(), 25);

		g.drawFittedText(contact_text, contact_text_bounds, juce::Justification::centred, 1, 1);
		g.drawFittedText(version_text, version_text_bounds, juce::Justification::centred, 1, 1);

	}
	else if (help_state) {
		help_state = false;
	}
}