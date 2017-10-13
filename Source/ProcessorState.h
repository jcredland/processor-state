/*
  ==============================================================================

    ProcessorState.h
    Created: 13 Oct 2017 10:32:31am
    Author:  jim

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

/**
* Like AudioProcessorValueTreeState only without
* - the horrible long name :)
* - the threading problems :)
* - the undo feature :(
*
* @note
* It must allow data to be loaded from any thread, and without the use of the
* message thread provide all necessary information to the audio processor. Some
* hosts may lock up the message thread when rendering.
* 
* I want to add a feature that allows non-parameter data to be set, saved, loaded
* and sent safely to the audio thread, but it's not here yet!
*/
class ProcessorState
    :
    public Timer
{
public:
    explicit ProcessorState (AudioProcessor& processor) : processor(processor) {}

    class Parameter;
    class SliderAttachment;

    /** Creates and returns a new parameter object for controlling a parameter
    with the given ID.

    Calling this will create and add a special type of AudioProcessorParameter to the
    AudioProcessor to which this state is attached.

    @param parameterID              A unique string ID for the new parameter
    @param parameterName            The name that the parameter will return from AudioProcessorParameter::getName()
    @param labelText                The label that the parameter will return from AudioProcessorParameter::getLabel()
    @param valueRange               A mapping that will be used to determine the value range which this parameter uses
    @param defaultValue             A default value for the parameter (in non-normalised units)
    @param valueToTextFunction      A function that will convert a non-normalised value to a string for the
    AudioProcessorParameter::getText() method. This can be nullptr to use the
    default implementation
    @param textToValueFunction      The inverse of valueToTextFunction
    @param isMetaParameter          Set this value to true if this should be a meta parameter
    @param isAutomatableParameter   Set this value to false if this parameter should not be automatable
    @param isDiscrete               Set this value to true to make this parameter take discrete values in a host.
    @see AudioProcessorParameter::isDiscrete

    @returns the parameter object that was created
    */
    Parameter* createAndAddParameter (const String& parameterID,
        const String& parameterName,
        const String& labelText,
        NormalisableRange<float> valueRange,
        float defaultValue,
        std::function<String  (float)> valueToTextFunction,
        std::function<float  (const String&)> textToValueFunction,
        bool isMetaParameter = false,
        bool isAutomatableParameter = true,
        bool isDiscrete = false);

    /**
    * Returns a ProcessorState::Parameter by its ID string.
    *
    * @note All functions that use the parameterID do a relatively slow hunt
    * for the parameter using string comparison.  So it's best to run them once
    * and then save the returned pointer, rather than calling them all the time
    * in your processBlock or something.
    */
    Parameter* getParameter (StringRef parameterID) const noexcept;

    /** Returns a pointer to a floating point representation of a particular
    * parameter which a realtime process can read to find out its current value.
    */
    float* getRawParameterValue (StringRef parameterID) const noexcept;

    ///** 
    // * Return some non-parameter data. It would be nice if this could be of any
    // * type, had callbacks to the UI when it was changed and was handled in a
    // * thread-safe manner when saving and loading.  This might include waveform
    // * settings, complex envelope data, sample file names etc.
    // */
    //int getRawNonParameterValue(StringRef nonParameterId) const noexcept;

    /**
    * Thread-safe, return the current state of the processor configuration.
    */
    ValueTree toValueTree () const;

    /**
    * Thread-safe restoration of plugin state from valuetree.  If values aren't
    * included we set them to the default value for the parameter.
    */
    void load(ValueTree) const;

private:
    void forEachParameter (std::function<void(int, Parameter*)> func) const;
    void timerCallback () override;

    AudioProcessor & processor;
};


/** */
class ProcessorState::Parameter : public AudioProcessorParameterWithID
{
public:
    ~Parameter ();

    float getValue () const override;
    float getDefaultValue () const override;
    float getValueForText (const String& text) const override;
    String getText (float v, int length) const override;
    int getNumSteps () const override;
    void setValue (float newValue) override;

    NormalisableRange<float> getRange() const { return range; }

    /** Can be called from ANY thread.  */
    void setUnnormalisedValue (float newUnnormalisedValue);

    bool isMetaParameter () const override;
    bool isAutomatable () const override;
    bool isDiscrete () const override;

    class Listener
    {
    public:
        virtual ~Listener ();
        /** Will only be called on the message thread */
        virtual void parameterChanged(const String & parameterId, float newValue) = 0;
    };

    void addListener (Listener* l);
    void removeListener (Listener* l);

    float value;

private:
    NormalisableRange<float> range;
    friend class ProcessorState;
    float defaultValue;

    Parameter (const String& parameterID, const String& paramName, const String& labelText,
        NormalisableRange<float> r, float defaultVal, std::function<String  (float)> valueToText,
        std::function<float  (const String&)> textToValue, bool meta, bool automatable, bool discrete);

    void callMessageThreadListeners ();

    ListenerList<Listener> listeners;
    std::function<String (float)> valueToTextFunction;
    std::function<float (const String&)> textToValueFunction;
    std::atomic<int> needsUpdate;
    bool listenersNeedCalling;
    const bool isMetaParam, isAutomatableParam, isDiscreteParam;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Parameter)
};

/**
* Connect a slider to a parameter.
*/
class SliderAttachment : Slider::Listener, ProcessorState::Parameter::Listener
{
public:
    SliderAttachment (ProcessorState& state, const String& paramID, Slider& slider)
        : slider (slider)
    {
        parameter = state.getParameter(paramID);

        /**
         * Asserts here? paramID was not valid.  All parameters must be created
         * before building the UI.
         */
        jassert(parameter);

        auto r{ parameter->getRange() };

        slider.setRange (r.start, r.end, r.interval);
        slider.setSkewFactor (r.skew, r.symmetricSkew);

        slider.setDoubleClickReturnValue (true, r.convertFrom0to1 (parameter->getDefaultValue()));
        slider.setValue(*state.getRawParameterValue(paramID), dontSendNotification);

        slider.addListener (this);
        parameter->addListener(this);

        updateControlValue();
    }

    ~SliderAttachment()
    {
        parameter->removeListener(this);
        slider.removeListener (this);
    }

private:
    void parameterChanged (const String&, float) override { updateControlValue(); }

    void updateControlValue ()
    {
        ScopedValueSetter<bool> svs (ignoreCallbacks, true);
        jassert(MessageManager::getInstance()->isThisTheMessageThread());

        slider.setValue (parameter->value, sendNotificationSync);
    }

    void sliderValueChanged (Slider* s) override
    {
        if (!ignoreCallbacks && !ModifierKeys::getCurrentModifiers().isRightButtonDown()) // why the right mouse button check?
            parameter->setUnnormalisedValue(float(s->getValue()));
    }

    void sliderDragStarted (Slider*) override { parameter->beginChangeGesture(); }
    void sliderDragEnded   (Slider*) override { parameter->endChangeGesture(); }

    Slider& slider;
    bool ignoreCallbacks{ false };
    ProcessorState::Parameter * parameter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SliderAttachment)
};
