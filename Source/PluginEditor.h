/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"


class ProcessorstateAudioProcessorEditor  
: 
public AudioProcessorEditor,
ProcessorState::Data::Listener,
Button::Listener
{
public:
    ProcessorstateAudioProcessorEditor (ProcessorstateAudioProcessor&);
    ~ProcessorstateAudioProcessorEditor();

    void paint (Graphics&) override;
    void resized() override;
    void updateButtonText ();

    void buttonClicked (Button*) override;

private:
    void processorStateDataChanged (const String&) override;

    ProcessorstateAudioProcessor& processor;
    Slider volumeSlider;
    ProcessorState::SliderAttachment volumeAttachment{ processor.state, "volume", volumeSlider };

    TextButton file;
    ProcessorStateFile * fileState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorstateAudioProcessorEditor)
};
