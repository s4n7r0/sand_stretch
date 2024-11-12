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
StretchAudioProcessorEditor::StretchAudioProcessorEditor(StretchAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), help_state{ false }, components{
    new components::AttachedTextButton("?"),
    new components::AttachedToggleButton(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::trigger]),
    new components::AttachedToggleButton(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::hold]),
    new components::AttachedSlider(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::offset]),
    new components::AttachedToggleButton(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::tempo_toggle]),
    new components::AttachedToggleButton(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::reverse]),
    new components::AttachedSlider(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::declick]),
    new components::AttachedSlider(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::grain]),
    new components::AttachedSlider(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::tempo]),
    new components::AttachedSlider(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::ratio]),
    new components::AttachedSlider(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::subd]),
    new components::AttachedSlider(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::zwindow]),
    new components::AttachedSlider(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::zoffset]),
    new components::AttachedSlider(p, stretch::PARAMS_STRING_IDS[PARAMS_IDS::crossfade])
    }
{
    using PID = PARAMS_IDS;
    using PASlider = AttachedSlider*;
    using PAToggle = AttachedToggleButton*;

    setSize(p.get_editor_width(), p.get_editor_height());
    setup();

    components[PID::help]->get()->setColour(1, colours::component_background);

    auto offset = dynamic_cast<PASlider>(components[PID::offset]);

    offset->param.setSliderStyle(juce::Slider::SliderStyle::LinearBarVertical);
    offset->param.setSliderSnapsToMousePosition(false);
    offset->param.setTextBoxIsEditable(false);
    offset->param.setColour(juce::Slider::trackColourId, colours::invisible);
    offset->param.setColour(juce::Slider::backgroundColourId, colours::thumb);

    auto declick = dynamic_cast<PASlider>(components[PID::declick]);

    declick->param.setSliderStyle(juce::Slider::SliderStyle::LinearBarVertical);
    declick->param.setSliderSnapsToMousePosition(false);
    declick->param.setTextBoxIsEditable(false);
    declick->param.setColour(juce::Slider::trackColourId, colours::invisible);
    declick->param.setColour(juce::Slider::backgroundColourId, colours::thumb);

    for (int i = PID::help; i < PID::end; ++i) {
        components[i]->set_bounds(components_bounds[i]);

        if (i >= PID::grain && i < PID::end) {
            juce::Range<double> range({ range_vector[i].getStart(), range_vector[i].getEnd() });
            PASlider slider = dynamic_cast<PASlider>(components[i]);

            int num_decimal = (i == PID::ratio) ? 2 : 0;
            slider->param.setNumDecimalPlacesToDisplay(num_decimal);
            slider->param.setTextBoxStyle(slider->param.getTextBoxPosition(), 1, 50, 25);
            slider->param.setRange(range, 0.01f);

            if (i == PID::tempo || i == PID::subd) {
                slider->param.setRange(range, 1);
                slider->param.setColour(juce::Slider::textBoxTextColourId, colours::invisible);
            }

            //fix by getting value from parameter not from component in stretcheditor
            //bug with this when changing params not from ui when tempo on
            //slider->param.onValueChange = [&]() { repaint(); };
        }

        addAndMakeVisible(components[i]->get(), 0);
    }

    auto hold = dynamic_cast<PAToggle>(components[PID::hold]);
    auto tempo_toggle = dynamic_cast<PAToggle>(components[PID::tempo_toggle]);
    auto reverse = dynamic_cast<PAToggle>(components[PID::reverse]);
    //hold->param.onStateChange = [&]() { repaint(); };    

    //paramid is tempo_toggle so adjust it 
    tempo_toggle->param.setButtonText("tempo");
    //reverse->param.setButtonText("reverse");
    //tempo_toggle->param.onStateChange = [&]() { repaint(); };
    //auto ratio = dynamic_cast<PASlider>(components[PID::ratio]);

    declick_text_bounds = StretchBounds(components[PID::declick]->get()->getBounds());
    grain_text_bounds = StretchBounds(components[PID::grain]->get()->getBounds(), -20);
    ratio_text_bounds = StretchBounds(components[PID::ratio]->get()->getBounds(), -20);
    zwindow_text_bounds = StretchBounds(components[PID::zwindow]->get()->getBounds(), -20);
    zoffset_text_bounds = StretchBounds(components[PID::zoffset]->get()->getBounds(), -20);
    crossfade_text_bounds = StretchBounds(components[PID::crossfade]->get()->getBounds(), -20);

    resized();

}

StretchAudioProcessorEditor::~StretchAudioProcessorEditor()
{
    //for (auto component : components) {
    //    delete component;
    //}
}

//==============================================================================
void StretchAudioProcessorEditor::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(colours::background);
    g.setColour(colours::text);
    g.setFont(monospace_font);

    set_font_size(g, 15 * abs_scale);

    draw_labels(g);

    draw_help(g);

    show_or_hide();

    draw_debug(g);

}

void StretchAudioProcessorEditor::resized()
{

    using PID = stretch::PARAMS_IDS;

    audioProcessor.set_editor_size(getWidth(), getHeight());

    set_scales();

    IRec temp_bounds;
    int abs_slider = int(slider_width / abs_scale);
    auto help = dynamic_cast<components::AttachedTextButton*>(components[PID::help]);

    components[PID::trigger]->get()->setTransform(juce::AffineTransform::scale(abs_scale, abs_scale));
    components[PID::hold]->get()->setTransform(juce::AffineTransform::scale(abs_scale, abs_scale));
    components[PID::offset]->get()->setTransform(juce::AffineTransform::scale(abs_scale, abs_scale));
    components[PID::tempo_toggle]->get()->setTransform(juce::AffineTransform::scale(abs_scale, abs_scale));
    components[PID::reverse]->get()->setTransform(juce::AffineTransform::scale(abs_scale, abs_scale));
    components[PID::declick]->get()->setTransform(juce::AffineTransform::scale(abs_scale, abs_scale));

    temp_bounds = help->original_bounds;
    help->param.setCentrePosition(0, 0);
    help->param.setBounds(temp_bounds);
    //i struggled way too much with this...
    help->param.setTransform(juce::AffineTransform::scale(abs_scale, abs_scale).translated(getWidth() - size_width * abs_scale, 0));

    for (int i = PID::grain; i < PID::end; ++i)
    {
        temp_bounds = components[i]->original_bounds;
        temp_bounds.setWidth(abs_slider);
        components[i]->get()->setBounds(temp_bounds);
        components[i]->get()->setTransform(juce::AffineTransform::scale(abs_scale, abs_scale));
    }

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
    help_texts.push_back(offset_text);
    help_texts.push_back(tempo_toggle_text);
    help_texts.push_back(reverse_text);
    help_texts.push_back(declick_text);
    help_texts.push_back(grain_text);
    help_texts.push_back(tempo_text);
    help_texts.push_back(ratio_text);
    help_texts.push_back(subd_text);
    help_texts.push_back(zcross_text);
    help_texts.push_back(zoffset_text);
    help_texts.push_back(crossfade_text);

    getLookAndFeel().setColour(ResizableWindow::backgroundColourId, colours::background);
    getLookAndFeel().setColour(SliderIds::backgroundColourId, colours::component_background);
    getLookAndFeel().setColour(SliderIds::textBoxTextColourId, colours::text);
    getLookAndFeel().setColour(SliderIds::trackColourId, colours::slider_track);
    getLookAndFeel().setColour(SliderIds::thumbColourId, colours::thumb);
    getLookAndFeel().setColour(SliderIds::textBoxOutlineColourId, colours::invisible);
    getLookAndFeel().setColour(TextButtonIds::buttonColourId, colours::component_background);
    getLookAndFeel().setColour(ToggleButton::textColourId, colours::text);
}