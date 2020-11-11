#pragma once

#include <exception>
#include <JuceHeader.h>
#include <memory>
#include <string>
#include "Autoencoder.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent,
                       private juce::MidiInputCallback
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    void openButtonClicked();
    void loadAutoencoder(const juce::String& path);
    void createSliders();
    void deleteSliders();
    void resetSliders();
    void setMidiInput(int index);
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage&) override;
    void postMessageToList(const juce::MidiMessage&, const juce::String&);
    void addMessageToList(const juce::MidiMessage&, const juce::String&);

    juce::AudioDeviceSelectorComponent adsc;
    juce::ToggleButton mStorePreset;
    std::vector<juce::Slider*> mSliders;
    std::vector<std::vector<float>> mSlidersMemory;
    std::vector<juce::TextButton*> mTextButtons;
    juce::TextButton openButton;
    juce::Slider xMaxSlider;
    juce::Slider sClipSlider;

    std::unique_ptr<Autoencoder> mAutoencoder;

    //==============================================================================
    // from handling midi events
    juce::ComboBox midiInputList;                     // [2]
    juce::Label midiInputListLabel;
    int lastInputIndex = 0;                           // [3]
    bool isAddingFromMidiInput = false;               // [4]

    double startTime;

    // This is used to dispach an incoming message to the message thread
    class IncomingMessageCallback   : public juce::CallbackMessage
    {
    public:
        IncomingMessageCallback (MainComponent* o, const juce::MidiMessage& m, const juce::String& s)
           : owner (o), message (m), source (s)
        {}

        void messageCallback() override
        {
            if (owner != nullptr)
                owner->addMessageToList (message, source);
        }

        Component::SafePointer<MainComponent> owner;
        juce::MidiMessage message;
        juce::String source;
    };

    static juce::String getMidiMessageDescription (const juce::MidiMessage& m)
    {
        if (m.isNoteOn())           return "Note on "          + juce::MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3);
        if (m.isNoteOff())          return "Note off "         + juce::MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3);
        if (m.isProgramChange())    return "Program change "   + juce::String (m.getProgramChangeNumber());
        if (m.isPitchWheel())       return "Pitch wheel "      + juce::String (m.getPitchWheelValue());
        if (m.isAftertouch())       return "After touch "      + juce::MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3) +  ": " + juce::String (m.getAfterTouchValue());
        if (m.isChannelPressure())  return "Channel pressure " + juce::String (m.getChannelPressureValue());
        if (m.isAllNotesOff())      return "All notes off";
        if (m.isAllSoundOff())      return "All sound off";
        if (m.isMetaEvent())        return "Meta event";

        if (m.isController())
        {
            juce::String name (juce::MidiMessage::getControllerName (m.getControllerNumber()));

            if (name.isEmpty())
                name = "[" + juce::String (m.getControllerNumber()) + "]";

            return "Controller " + name + ": " + juce::String (m.getControllerValue());
        }

        return juce::String::toHexString (m.getRawData(), m.getRawDataSize());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
