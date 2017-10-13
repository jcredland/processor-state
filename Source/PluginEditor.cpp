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

    fileState = dynamic_cast<ProcessorStateFile*>(p.state.getData("file"));
    fileState->addListener(this);
    updateButtonText();
    file.addListener(this);
    addAndMakeVisible(file);
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
    file.setBounds(b.removeFromTop(25));
}

void ProcessorstateAudioProcessorEditor::updateButtonText ()
{
    file.setButtonText(fileState->getFile().getFileNameWithoutExtension());
}

void ProcessorstateAudioProcessorEditor::buttonClicked (Button*)
{
    FileChooser chooser{ "Find audio" };

    if (chooser.browseForFileToOpen())
        fileState->setFile(chooser.getResult(), sendNotification);
}

void ProcessorstateAudioProcessorEditor::processorStateDataChanged (const String&)
{
    updateButtonText();
}
