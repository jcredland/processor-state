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

    return parametersTree;
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
        if (p->needsUpdate.compareAndSetBool(0, 1))
        {
            p->callMessageThreadListeners();
            anythingUpdated = true;
        }
    });

    startTimer(anythingUpdated ? 1000 / 50
        : jlimit(50, 500, getTimerInterval() + 20));
}