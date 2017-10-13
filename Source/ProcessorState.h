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
* Manages access to audio processor configuration information including
* parameters and non-parameter data.
*
* 
* 
*/
class ProcessorState
    :
    public Timer
{
public:
    explicit ProcessorState (AudioProcessor& processor) : processor(processor) { startTimerHz(10); }

    void notifyChangedData () const { processor.updateHostDisplay(); }

    class Parameter;
    class SliderAttachment;
    class Data;

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

    THREADING SPEC: All calls to createAndAddParameter must take place during the
    constructor of the PluginProcessor.
    */
    Parameter* createAndAddParameter (const String& parameterID,
        const String& parameterName,
        const String& labelText,
        NormalisableRange<float> valueRange,
        float defaultValue,
        std::function<String (float)> valueToTextFunction,
        std::function<float (const String&)> textToValueFunction,
        bool isMetaParameter = false,
        bool isAutomatableParameter = true,
        bool isDiscrete = false);


    /** 
     * Add a data item which will be saved and loaded with the plugin
     * parameters. 
     * 
     * THREADING SPEC: For the other functions to be threadsafe addData calls
     * should all be completed before the end of your AudioProcessor
     * constructor. 
     */
    void addData(Data * data);

    /** 
     * Return a data item, which you will probably want to dyanmic_cast into your 
     * actual data item type.  
     */
    Data * getData(StringRef dataID) const noexcept;

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

    /**
    * Thread-safe, return the current state of the processor configuration.
    */
    ValueTree toValueTree () const;

    /**
    * Thread-safe restoration of plugin state from valuetree.  If values aren't
    * included we set them to the default value for the parameter.
    *
    * THREADING SPEC: It must allow data to be loaded from any thread and,
    * without the use of the message thread (which may be locked by the host),
    * provide all necessary information to the audio processor.
    */
    void load(ValueTree) const;

    /** 
     * Save the ProcessorState to the memory block.
     * 
     * Call from your AudioProcessor::getStateInformation call. Use instead of 
     * ProcessorState::toValueTree()
     *
     * THREADING SPEC: supports being called from any thread.
     */
    void getStateInformation (MemoryBlock& destData) const;

    /** 
     * Load the ProcessorState from the memory block.
     * 
     * Call from your AudioProcessor::setStateInformation call.  Use instead of 
     * ProcessorState::load()
     */
    void setStateInformation (const void* data, int sizeInBytes);


private:
    OwnedArray<Data> dataItems;
    void forEachParameter (std::function<void(int, Parameter*)> func) const;
    void timerCallback () override;

    AudioProcessor & processor;
};


/** 
 * An implementation of AudioProcessorParameterWithID for the ProcessorState.  Normally
 * you won't need to interact with this object directly.  Instead use
 * getRawParameterValue(), createAndAddParameter() and the SliderAttachment class.
 * 
 * You may need to use this if you are creating new Attachment classes.
 */
class ProcessorState::Parameter : public AudioProcessorParameterWithID
{
public:
    ~Parameter ();

    /** Returns the normalised value */
    float getValue () const override;

    /** Returns the _unnormalised_ default value */
    float getDefaultValue () const override;

    float getValueForText (const String& text) const override;

    String getText (float v, int length) const override;

    int getNumSteps () const override;

    /** 
     * Set the normalized value. Called by the host.
     * 
     * THREADING SPEC: Can be called from any thread.
     */
    void setValue (float newValue) override;

    /** 
     * Set the unnormalised value.  
     *
     * THREADING SPEC: Can be called from any thread.  
     */
    void setUnnormalisedValue (float newUnnormalisedValue);

    NormalisableRange<float> getRange() const { return range; }

    bool isMetaParameter () const override;
    bool isAutomatable () const override;
    bool isDiscrete () const override;

    class Listener
    {
    public:
        virtual ~Listener ();
        /** 
         * Called when the host changes the parameter value, use this to trigger
         * a UI update.  Usually this is done for you using an Attachment
         * object such as ProcessorState::SliderAttachment
         * 
         * THREADING SPEC: Will only be called on the message thread 
         */
        virtual void parameterChanged(const String & parameterId, float newValue) = 0;
    };

    void addListener (Listener* l);
    void removeListener (Listener* l);

    float value; /**< should this be a std::atomic<float> really? */

private:
    friend class ProcessorState;

    Parameter (const String& parameterID, const String& paramName, const String& labelText,
        NormalisableRange<float> r, float defaultVal, std::function<String (float)> valueToText,
        std::function<float (const String&)> textToValue, bool meta, bool automatable, bool discrete);

    void callMessageThreadListeners ();

    NormalisableRange<float> range;
    float defaultValue;
    ListenerList<Listener> listeners;
    std::function<String (float)> valueToTextFunction;
    std::function<float (const String&)> textToValueFunction;
    std::atomic<int> needsUpdate;
    const bool isMetaParam, isAutomatableParam, isDiscreteParam;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Parameter)
};

/**
 * Base class for classes containing data saved with the preset but not exposed
 * as a parameter.
 * 
 * You should override this class for specific data types.
 * 
 * wrapper around other data that the plugin may want to use.  This is data that
 * isn't suitable for use as a parameter, e.g. a sample filename, a complete
 * sampler zone map or perhaps a complex envelope shape.
 */
class ProcessorState::Data : AsyncUpdater
{
public:
    ProcessorState::Data(ProcessorState & state, const String & dataID): state(state), dataID(dataID) {}
    virtual ~Data () = default;

    String getDataID () const { return dataID; }

    class Listener
    {
    public:
        virtual ~Listener() {}
        /** 
         * THEADING SPEC: Will only be called on the message thread. Use to
         * update your UI object when the state has changed (typically as a
         * result of the host calling setStateInformation).
         */
        virtual void processorStateDataChanged(const String & dataID) = 0;
    };

    void addListener(Listener * l) { listeners.add(l); }
    void removeListener(Listener * l) { listeners.remove(l); }

protected:
    /** Save the contents of your implementation to a ValueTree.
     *
     * This function may be called on any thread. It should serialize the 
     * state of your ProcessorState::Data object into a ValueTree.
     *
     * THREADING SPEC
     * - This function may be called on any thread.
     */
    virtual ValueTree serialize() = 0;

    /** Called with data you previously created with serialize()
     *
     * It should not return until your plugin is ready to play with the new
     * data.
     *
     * It should:
     *  - store the data
     *  - reconfigure the processor as needed using the data
     *  - notify the UI to update by calling Data::notifyChanged(...).
     *
     * THREADING SPEC
     * - This function may be called on any thread.
     * - You will need to use a lock or other thread safety mechanisms when
     *   changing certain types of data.  
     * - You should not rely on the availablity of the message thread for
     *   handling updates.  
     */
    virtual bool deserialize(ValueTree valuetree) = 0;

    /**
     * Called when a preset is loaded that doesn't include this data.  Use it to
     * clear the state to a sensible default.    The implementation will
     * probably be very similar to the deserialize function only with some
     * default settings (includign saving, reconfiguring and notifying the UI).
     *
     * THREADING SPEC
     * - This function may be called on any thread.
     */
    virtual void setToDefaultState () = 0;

    /** 
     * Call from your implementation when the data has changed (e.g. the user
     * changed the UI and the state may need saving.  
     *
     * THREADING SPEC: may be called from any thread**.
     * 
     * ** need to check updateHostDisplay
     */
    void notifyChanged (NotificationType notifyMessageThreadListeners);

private:
    friend class ProcessorState;

    /** @internal - triggers a call to the listeners. */
    void handleAsyncUpdate () override;

    ProcessorState & state;
    ListenerList<Listener> listeners;
    String dataID;
    std::atomic<int> needsUpdate;
};


/**
 * A simple example showing how a File object could be updated in a threadsafe
 * manner.  In this case we are expecting actionOnChange to occur on either the
 * current thread or a file loading thread and not to return until the file load
 * has finished.
 * 
 * This is an easy example.  A more complex object might include an entire
 * sampler configuration.
 */
class ProcessorStateFile : public ProcessorState::Data
{
public:
    /**
     * @param state           the shared ProcessorState
     * @param dataID          the dataID for this ProcessorState::Data object
     * @param actionOnChange  this might be called on any thread
     */
    ProcessorStateFile(ProcessorState & state, const String & dataID, std::function<void(const File & action)> actionOnChange)
    : 
    Data(state, dataID), 
    actionOnChange(actionOnChange)
    {}

    /** Call from the UI when the user selects another file. */
    void setFile(const File & newFile, NotificationType uiNotificationType)
    {
        ScopedLock l(criticalSection);

        if (file != newFile)
        {
            file = newFile;
            actionOnChange(file);
            notifyChanged(uiNotificationType);
        }
    }

    /** Call from the UI to display the current file name to the user. */
    File getFile() const
    {
        ScopedLock l(criticalSection);
        return file;
    }

protected:
    void setToDefaultState () override
    {
        setFile(File(), sendNotification);
    }

    bool deserialize (ValueTree valuetree) override
    {
        if (valuetree.getType() != Identifier("ProcessorStateFile"))
            return false;

        setFile(File(valuetree["file"].toString()), sendNotification);

        return true;
    }

    ValueTree serialize () override
    {
        ScopedLock l(criticalSection);
        ValueTree t{ "ProcessorStateFile" };
        t.setProperty("file", file.getFullPathName(), nullptr);
        return t;
    }

    File file;
    CriticalSection criticalSection;
    std::function<void(const File& action)> actionOnChange;
};

/**
* Connect a slider to a parameter.
*/
class ProcessorState::SliderAttachment : Slider::Listener, Parameter::Listener
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
    Parameter * parameter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SliderAttachment)
};
