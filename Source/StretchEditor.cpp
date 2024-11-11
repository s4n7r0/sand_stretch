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

	auto using_tempo = dynamic_cast<components::AttachedToggleButton*>(components[PARAMS_IDS::tempo_toggle]);
	auto using_hold = (bool)audioProcessor.apvts.getRawParameterValue(PARAMS_STRING_IDS[PARAMS_IDS::hold])->load();
	auto grain_comp = components[PARAMS_IDS::grain]->get();
	auto tempo_comp = components[PARAMS_IDS::tempo]->get();
	auto ratio_comp = components[PARAMS_IDS::ratio]->get();
	auto subd_comp = components[PARAMS_IDS::subd]->get();

	//ik this is ugly but works
	if (!help_state) {
		if (using_hold) {
			if (using_tempo->param.getToggleState()) {
				grain_comp->setVisible(false);
				tempo_comp->setVisible(true);
				ratio_comp->setVisible(false);
				subd_comp->setVisible(true);
			}
			else {
				grain_comp->setVisible(true);
				tempo_comp->setVisible(false);
				ratio_comp->setVisible(true);
				subd_comp->setVisible(false);
			}
		}
		else {
			if (using_tempo->param.getToggleState()) {
				grain_comp->setVisible(false);
				tempo_comp->setVisible(true);
				ratio_comp->setVisible(true);
				subd_comp->setVisible(false);
			}
			else {
				grain_comp->setVisible(true);
				tempo_comp->setVisible(false);
				ratio_comp->setVisible(true);
				subd_comp->setVisible(false);
			}
		}
	}

}

void StretchAudioProcessorEditor::set_scales() {
	x_scale = getWidth() / size_width;
	y_scale = getHeight() / size_height;
	abs_scale = std::log2f(std::fabs(((size_width - getWidth()) / size_width) * ((size_height - getHeight()) / size_height)) + 2.f);
	slider_width = getWidth() - (size_width / 6) * abs_scale;
}

void StretchAudioProcessorEditor::draw_labels(juce::Graphics& g) {

	using namespace stretch;

	auto tempo_toggle = dynamic_cast<components::AttachedToggleButton*>(components[PARAMS_IDS::tempo_toggle]);
	auto using_hold = (bool)audioProcessor.apvts.getRawParameterValue(PARAMS_STRING_IDS[PARAMS_IDS::hold])->load();

	IRec temp_bounds = grain_text_bounds.bounds;
	temp_bounds *= abs_scale;
	temp_bounds.setWidth(slider_width);

	IRec tempo_bounds = IRec({ 25, 50, 50, 50 }) *= abs_scale;
	auto tempo_comp = dynamic_cast<components::AttachedSlider*>(components[PARAMS_IDS::tempo]);
	float tempo_val = tempo_comp->param.getValue();

	String tempo_text(String().formatted("1/%.f", std::pow(2, MAX_TEMPO_SIZE - tempo_val)));

	if (tempo_toggle->param.getToggleState()) {
		g.drawFittedText(juce::String("tempo"), temp_bounds, juce::Justification::right, 4, 0);
		set_font_size(g, 13 * abs_scale);
		g.drawFittedText(tempo_text, tempo_bounds, juce::Justification::centred, 1, 1.f);
		set_font_size(g, 15 * abs_scale);
	}
	else {
		g.drawFittedText(juce::String("grain"), temp_bounds, juce::Justification::right, 4, 0);
	}

	temp_bounds = ratio_text_bounds.bounds;
	temp_bounds *= abs_scale;
	temp_bounds.setWidth(slider_width);

	IRec subdivision_bounds = IRec({ 17, 50 + 50, 65, 50 }) *= abs_scale;
	auto subdivision_comp = dynamic_cast<components::AttachedSlider*>(components[PARAMS_IDS::subd]);
	float subdivision_val = subdivision_comp->param.getValue();

	String subdivision_text("");
	if (subdivision_val == 1) subdivision_text = "triplet";
	else if (subdivision_val == 2) subdivision_text = "dotted";

	if (tempo_toggle->param.getToggleState()) {
		if (using_hold) {
			g.drawFittedText(juce::String("subdivision"), temp_bounds, juce::Justification::right, 4, 0);
			set_font_size(g, 13 * abs_scale);
			g.drawFittedText(subdivision_text, subdivision_bounds, juce::Justification::centred, 1, 1.f);
			set_font_size(g, 15 * abs_scale);
		}
		else {
			g.drawFittedText(juce::String("ratio"), temp_bounds, juce::Justification::right, 4, 0);
		}
	}
	else {
		g.drawFittedText(juce::String("ratio"), temp_bounds, juce::Justification::right, 4, 0);
	}

	temp_bounds = zwindow_text_bounds.bounds;
	temp_bounds *= abs_scale;
	temp_bounds.setWidth(slider_width);

	g.drawFittedText(juce::String("zcross size"), temp_bounds, juce::Justification::right, 1, 0);	
	
	temp_bounds = zoffset_text_bounds.bounds;
	temp_bounds *= abs_scale;
	temp_bounds.setWidth(slider_width);

	g.drawFittedText(juce::String("zcross offset"), temp_bounds, juce::Justification::right, 1, 0);

	temp_bounds = crossfade_text_bounds.bounds;
	temp_bounds *= abs_scale;
	temp_bounds.setWidth(slider_width);

	g.drawFittedText(juce::String("crossfade"), temp_bounds, juce::Justification::right, 1, 0);

	temp_bounds = declick_text_bounds.bounds;
	temp_bounds.translate(-40, 0);
	temp_bounds *= abs_scale;
	//temp_bounds.setWidth(slider_width);
	set_font_size(g, 13 * abs_scale);

	g.drawFittedText(juce::String("declick"), temp_bounds, juce::Justification::centred, 1, 0);

	set_font_size(g, 15 * abs_scale);

	repaint();
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


void StretchAudioProcessorEditor::draw_debug(juce::Graphics& g) {

	using namespace stretch;

	set_font_size(g, 15 * abs_scale);

	IRec debug_bound(25, 200, size_width, 25);

	for (int i = 0; i < 5; ++i) {
		debug_bound.setY(315 + 25 * i);
		g.drawFittedText(audioProcessor.stretch_processor.debug_strings[i], debug_bound , juce::Justification::left, 1, 0);
	}

}
