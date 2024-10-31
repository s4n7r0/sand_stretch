/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "StretchEditor.h"

using namespace stretch;
//==============================================================================
StretchAudioProcessorEditor::StretchAudioProcessorEditor (StretchAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), help_state{ false }, components{
    new components::AttachedTextButton  ("?"),
    new components::AttachedToggleButton(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::trigger]),
    new components::AttachedToggleButton(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::hold]),
    new components::AttachedSlider      (p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::grain]),
    new components::AttachedSlider      (p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::ratio])
    }
{
    using PID = PARAMS_IDS;

    setSize (400, 300);
    setup();

    components[PID::help]->get()->setColour(1, colours::component_background);

    for (int i = PID::help; i < PID::end; ++i) {
        components[i]->set_bounds(components_bounds[i]);

        if (i >= PID::grain && i <= PID::end) {
            juce::Range<double> range({ range_vector[i].getStart(), range_vector[i].getEnd() });
            components::AttachedSlider* slider = dynamic_cast<components::AttachedSlider*>(components[i]);
            int num_decimal = (i == PID::grain) ? 0 : 2;
            slider->param.setNumDecimalPlacesToDisplay(num_decimal);
            slider->param.setTextBoxStyle(slider->param.getTextBoxPosition(), 1, 50, 25);
            slider->param.setRange(range, 0.01f);
        }

        addAndMakeVisible(components[i]->get(), 0);
    }

    grain_text_bounds = StretchBounds(components[PID::grain]->get()->getBounds(), -20);
    ratio_text_bounds = StretchBounds(components[PID::ratio]->get()->getBounds(), -20);
    
    resized();

}

StretchAudioProcessorEditor::~StretchAudioProcessorEditor()
{
}

//==============================================================================
void StretchAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(colours::background);
    g.setColour(colours::text);
    g.setFont(monospace_font);

    set_font_size(g, 15 * abs_scale);

    draw_labels(g);

    draw_help(g);

    show_or_hide();

}

void StretchAudioProcessorEditor::resized()
{

    using PID = stretch::PARAMS_IDS;

    audioProcessor.set_editor_size(getWidth(), getHeight());

    set_scales();

    IRec temp_bounds;
    int abs_slider = int(slider_width / abs_scale);
    components::AttachedTextButton* help = dynamic_cast<components::AttachedTextButton*>(components[PID::help]);

    components[PID::trigger]->get()->setTransform(juce::AffineTransform::scale(abs_scale, abs_scale));
    components[PID::hold]->get()->setTransform(juce::AffineTransform::scale(abs_scale, abs_scale));

    temp_bounds = help->original_bounds;
    help->param.setCentrePosition(0, 0);
    help->param.setBounds(temp_bounds);
    //i struggled way too much with this...
    help->param.setTransform(juce::AffineTransform::scale(abs_scale, abs_scale).translated(getWidth() - 400 * abs_scale, 0));

    //temp_bounds = components[PID::help]->original_bounds * abs_scale;
    //temp_bounds.setX(getWidth() - 35 * abs_scale);
    //components[PID::help]->get()->setBounds(temp_bounds);
    //help->param.settex

    for (int i = PID::grain; i < PID::end; ++i) 
    {
        temp_bounds = components[i]->original_bounds;
        temp_bounds.setWidth(abs_slider);
        components[i]->get()->setBounds(temp_bounds);
        components[i]->get()->setTransform(juce::AffineTransform::scale(abs_scale, abs_scale));
    }

    //temp_bounds = pitchmax.original_bounds;
    //pitchmax.slider.setCentrePosition(0, 0);
    //pitchmax.slider.setBounds(temp_bounds);
    ////i struggled way too much with this...
    //pitchmax.slider.setTransform(juce::AffineTransform::scale(abs_scale, abs_scale).translated(getWidth() - 400 * abs_scale, 0));

}

inline void StretchAudioProcessorEditor::setup() {

    using SliderIds = juce::Slider::ColourIds;
    using TextButtonIds = juce::TextButton::ColourIds;

    //setSize(size_width, size_height);
    setResizeLimits(size_width, size_height, size_width * 4, size_height * 4); //why would anyone want it this large...
    setResizable(true, true);
    setRepaintsOnMouseActivity(true);

    help_texts.push_back(trigger_text);
    help_texts.push_back(hold_text);
    help_texts.push_back(grain_text);
    help_texts.push_back(ratio_text);
    help_texts.push_back(zcross_text);

    getLookAndFeel().setColour(ResizableWindow::backgroundColourId, colours::background);
    getLookAndFeel().setColour(SliderIds::backgroundColourId, colours::component_background);
    getLookAndFeel().setColour(SliderIds::trackColourId, colours::slider_track);
    getLookAndFeel().setColour(SliderIds::thumbColourId, colours::thumb);
    getLookAndFeel().setColour(SliderIds::textBoxOutlineColourId, colours::invisible);
    getLookAndFeel().setColour(TextButtonIds::buttonColourId, colours::component_background);
}
