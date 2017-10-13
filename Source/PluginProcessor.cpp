/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
ProcessorstateAudioProcessor::ProcessorstateAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    volumeValue = &state.createAndAddParameter("volume", "volume", "Volume", { 0.0, 2.0 }, { 1.0 }, nullptr, nullptr)->value;
}

ProcessorstateAudioProcessor::~ProcessorstateAudioProcessor()
{
}

//==============================================================================
const String ProcessorstateAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ProcessorstateAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ProcessorstateAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ProcessorstateAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ProcessorstateAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ProcessorstateAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ProcessorstateAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ProcessorstateAudioProcessor::setCurrentProgram (int index)
{
}

const String ProcessorstateAudioProcessor::getProgramName (int index)
{
    return {};
}

void ProcessorstateAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void ProcessorstateAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void ProcessorstateAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ProcessorstateAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ProcessorstateAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto data = buffer.getArrayOfWritePointers();
    auto numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
        for (int i = 0; i < numSamples; ++i)
            data[channel][i] = *volumeValue * Random::getSystemRandom().nextFloat();
}

//==============================================================================
bool ProcessorstateAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* ProcessorstateAudioProcessor::createEditor()
{
    return new ProcessorstateAudioProcessorEditor (*this);
}

//==============================================================================
void ProcessorstateAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    state.getStateInformation(destData);
}

void ProcessorstateAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    state.setStateInformation(data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProcessorstateAudioProcessor();
}
