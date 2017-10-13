/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"


//==============================================================================
/**
*/
class ProcessorstateAudioProcessorEditor  : public AudioProcessorEditor
{
public:
    ProcessorstateAudioProcessorEditor (ProcessorstateAudioProcessor&);
    ~ProcessorstateAudioProcessorEditor();

    void paint (Graphics&) override;
    void resized() override;

private:
    ProcessorstateAudioProcessor& processor;
    Slider volumeSlider;
    ProcessorState::SliderAttachment volumeAttachment{ processor.state, "volume", volumeSlider };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorstateAudioProcessorEditor)
};
