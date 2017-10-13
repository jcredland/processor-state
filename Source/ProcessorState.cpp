/*
  ==============================================================================

    ProcessorState.cpp
    Created: 13 Oct 2017 10:32:31am
    Author:  jim

  ==============================================================================
*/

#include "ProcessorState.h"

ProcessorState::Parameter* ProcessorState::createAndAddParameter (const String& parameterID, const String& parameterName, const String& labelText,
    NormalisableRange<float> valueRange, float defaultValue, std::function<String (float)> valueToTextFunction,
    std::function<float (const String&)> textToValueFunction, bool isMetaParameter, bool isAutomatableParameter, bool isDiscrete)
{
#if ! JUCE_LINUX
    jassert (MessageManager::getInstance()->isThisTheMessageThread());
#endif

    Parameter* p = new Parameter(parameterID, parameterName, labelText, valueRange,
        defaultValue, valueToTextFunction, textToValueFunction,
        isMetaParameter, isAutomatableParameter,
        isDiscrete);
    processor.addParameter(p);
    return p;
}

ProcessorState::Parameter* ProcessorState::getParameter (StringRef parameterID) const noexcept
{
    const int numParams = processor.getParameters().size();

    for (int i = 0; i < numParams; ++i)
    {
        AudioProcessorParameter* const ap = processor.getParameters().getUnchecked(i);

        // When using this class, you must allow it to manage all the parameters in your AudioProcessor, and
        // not add any parameter objects of other types!
        jassert (dynamic_cast<Parameter*> (ap) != nullptr);

        Parameter* const p = static_cast<Parameter*>(ap);

        if (parameterID == p->paramID)
            return p;
    }

    return nullptr;
}

float* ProcessorState::getRawParameterValue (StringRef parameterID) const noexcept
{
    if (auto p = getParameter(parameterID))
        return &p->value;

    return nullptr;
}

ValueTree ProcessorState::toValueTree () const
{
    ValueTree root{ "state" };
    auto parametersTree = root.getOrCreateChildWithName("parameters", nullptr);

    forEachParameter([&parametersTree](int, Parameter * p)
    {
        ValueTree child{ "PARAM" };
        child.setProperty("id", p->paramID, nullptr);
        child.setProperty("value", p->value, nullptr);
        parametersTree.addChild(child, -1, nullptr);
    });

    return root;
}

void ProcessorState::load (ValueTree root) const
{
    auto parametersTree = root.getOrCreateChildWithName("parameters", nullptr);

    forEachParameter([parametersTree](int, Parameter * p)
    {
        auto child = parametersTree.getChildWithProperty("id", p->paramID);

        if (child.isValid())
            p->setUnnormalisedValue(child["value"]);
        else
            p->setUnnormalisedValue(p->getDefaultValue());
    });
}

void ProcessorState::getStateInformation (MemoryBlock& destData) const
{
    ScopedPointer<XmlElement> xml (toValueTree().createXml());
    AudioProcessor::copyXmlToBinary (*xml, destData);
}

void ProcessorState::setStateInformation (const void* data, int sizeInBytes)
{
    ScopedPointer<XmlElement> xmlState (AudioProcessor::getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName ("state"))
            load(ValueTree::fromXml (*xmlState));
}

void ProcessorState::forEachParameter (std::function<void(int, Parameter*)> func) const
{
    const int numParams = processor.getParameters().size();

    for (int i = 0; i < numParams; ++i)
    {
        AudioProcessorParameter* const ap = processor.getParameters().getUnchecked(i);
        jassert (dynamic_cast<Parameter*> (ap) != nullptr);

        Parameter* p = static_cast<Parameter*>(ap);

        func(i, p);
    }

}

void ProcessorState::timerCallback ()
{
    bool anythingUpdated = false;

    forEachParameter([&](int, Parameter * p)
    {
        int expected = 1;

        if (p->needsUpdate.compare_exchange_weak(expected, 0, std::memory_order_acquire))
        {
            p->callMessageThreadListeners();
            anythingUpdated = true;
        }
    });

    startTimer(anythingUpdated ? 1000 / 50
        : jlimit(50, 500, getTimerInterval() + 20));
}

ProcessorState::Parameter::~Parameter ()
{
    // should have detached all callbacks before destroying the parameters!
    jassert (listeners.size() <= 1);
}

float ProcessorState::Parameter::getValue () const
{
    return range.convertTo0to1(value);
}

float ProcessorState::Parameter::getDefaultValue () const
{
    return range.convertTo0to1(defaultValue);
}

int ProcessorState::Parameter::getNumSteps () const
{
    if (range.interval > 0)
        return (static_cast<int>((range.end - range.start) / range.interval) + 1);

    return AudioProcessor::getDefaultNumParameterSteps();
}

void ProcessorState::Parameter::setValue (float newValue)
{
    DBG("setValue" + String(newValue));
    newValue = range.snapToLegalValue(range.convertFrom0to1(newValue));

    if (value != newValue || listenersNeedCalling)
    {
        value = newValue;

        //listeners.call (&AudioProcessorValueTreeState::Listener::parameterChanged, paramID, value);
        listenersNeedCalling = false;

        needsUpdate.store(1, std::memory_order_release);
    }
}

void ProcessorState::Parameter::setUnnormalisedValue (float newUnnormalisedValue)
{
    DBG("setUnnormalisedValue" + String(newUnnormalisedValue));
    if (value != newUnnormalisedValue)
    {
        const float newValue = range.convertTo0to1(newUnnormalisedValue);
        setValueNotifyingHost(newValue);
    }
}

bool ProcessorState::Parameter::isMetaParameter () const
{
    return isMetaParam;
}

bool ProcessorState::Parameter::isAutomatable () const
{
    return isAutomatableParam;
}

bool ProcessorState::Parameter::isDiscrete () const
{
    return isDiscreteParam;
}

ProcessorState::Parameter::Listener::~Listener ()
{
}

void ProcessorState::Parameter::addListener (Listener* l)
{
    listeners.add(l);
}

void ProcessorState::Parameter::removeListener (Listener* l)
{
    listeners.remove(l);
}

void ProcessorState::Parameter::callMessageThreadListeners ()
{
    jassert(MessageManager::getInstance()->isThisTheMessageThread());
    listeners.call(&Listener::parameterChanged, paramID, value);
}

float ProcessorState::Parameter::getValueForText (const String& text) const
{
    return range.convertTo0to1(textToValueFunction != nullptr ? textToValueFunction(text)
                                   : text.getFloatValue());
}

String ProcessorState::Parameter::getText (float v, int length) const
{
    return valueToTextFunction != nullptr ? valueToTextFunction(range.convertFrom0to1(v))
               : AudioProcessorParameter::getText(v, length);
}

ProcessorState::Parameter::Parameter (const String& parameterID, const String& paramName, const String& labelText, NormalisableRange<float> r, float defaultVal, std::function<String (float)> valueToText, std::function<float (const String&)> textToValue, bool meta, bool automatable, bool discrete):
    AudioProcessorParameterWithID(parameterID, paramName, labelText),
    range(r), value(defaultVal),
    defaultValue(defaultVal), valueToTextFunction(valueToText), textToValueFunction(textToValue),
    listenersNeedCalling(true),
    isMetaParam(meta),
    isAutomatableParam(automatable),
    isDiscreteParam(discrete)
{
    needsUpdate.store(1, std::memory_order_release);
}
