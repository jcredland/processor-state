/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
ProcessorstateAudioProcessorEditor::ProcessorstateAudioProcessorEditor (ProcessorstateAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    addAndMakeVisible(volumeSlider);
    setSize (400, 300);
}

ProcessorstateAudioProcessorEditor::~ProcessorstateAudioProcessorEditor()
{
}

//==============================================================================
void ProcessorstateAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::grey);
}

void ProcessorstateAudioProcessorEditor::resized()
{
    auto b = getLocalBounds();
    volumeSlider.setBounds(b.removeFromTop(20));
}
