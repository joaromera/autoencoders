#include "MainComponent.h"
#define n_sliders 8
#define slider_ccnum 1

float inline linearmap(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//==============================================================================
MainComponent::MainComponent()
    : adsc(deviceManager, 2, 2, 2, 2, false, false, false, false), startTime(juce::Time::getMillisecondCounterHiRes() * 0.001)
{
    addAndMakeVisible(adsc);
    adsc.setBounds(900, 25, 300, 300);

    // Open button
    openButton.setButtonText("Open...");
    openButton.onClick = [this] { openButtonClicked(); };
    addAndMakeVisible(&openButton);

    // X Max Slider
    xMaxSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    xMaxSlider.setRange(0, 100, 1);
    xMaxSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    xMaxSlider.setPopupDisplayEnabled(true, false, this);
    xMaxSlider.setTextValueSuffix(" xMax value");
    xMaxSlider.setValue(0.0, juce::dontSendNotification);
    xMaxSlider.onValueChange = [this] {
        DBG("[MAINCOMPONENT] xMaxSlider: new value " << xMaxSlider.getValue());
        if (mAutoencoder)
            mAutoencoder->setXMax(xMaxSlider.getValue());
    };
    addAndMakeVisible(&xMaxSlider);

    // S Clip Slider
    sClipSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sClipSlider.setRange(-100, 0, 1);
    sClipSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    sClipSlider.setPopupDisplayEnabled(true, false, this);
    sClipSlider.setTextValueSuffix(" sClip value");
    sClipSlider.setValue(-100.0, juce::dontSendNotification);
    sClipSlider.onValueChange = [this] {
        DBG("[MAINCOMPONENT] sClipSlider: new value " << sClipSlider.getValue());
        if (mAutoencoder)
            mAutoencoder->setSClip(sClipSlider.getValue());
    };
    addAndMakeVisible(&sClipSlider);

    for (int i = 0; i < 8; ++i)
    {
        auto *s = new juce::Slider();
        s->setRange(-5.0, 5.0, 0.01);
        s->setPopupMenuEnabled(true);
        s->setValue(0, juce::dontSendNotification);
        s->setSliderStyle(juce::Slider::LinearVertical);
        s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
        s->setDoubleClickReturnValue(true, 0.0f); // double-clicking this slider will set it to 50.0
        s->onValueChange = [this, i, s] {
            DBG("[MAINCOMPONENT] slider: " << i << " new value: " << s->getValue());
            if (mAutoencoder)
                mAutoencoder->setInputLayers(i, s->getValue());
        };
        addAndMakeVisible(s);
        mSliders.push_back(s);
    }

    // MIDI INPUT
    addAndMakeVisible(midiInputList);
    midiInputList.setTextWhenNoChoicesAvailable("No MIDI Inputs Enabled");
    auto midiInputs = juce::MidiInput::getAvailableDevices();

    juce::StringArray midiInputNames;

    for (const auto &input : midiInputs)
        midiInputNames.add(input.name);

    midiInputList.addItemList(midiInputNames, 1);
    midiInputList.onChange = [this] { setMidiInput(midiInputList.getSelectedItemIndex()); };

    // find the first enabled device and use that by default
    for (const auto &input : midiInputs)
    {
        if (deviceManager.isMidiInputDeviceEnabled(input.identifier))
        {
            setMidiInput(midiInputs.indexOf(input));
            break;
        }
    }

    // if no enabled devices were found just use the first one in the list
    if (midiInputList.getSelectedId() == 0)
        setMidiInput(0);

    addAndMakeVisible(midiMessagesBox);
    midiMessagesBox.setMultiLine(true);
    midiMessagesBox.setReturnKeyStartsNewLine(true);
    midiMessagesBox.setReadOnly(true);
    midiMessagesBox.setScrollbarsShown(true);
    midiMessagesBox.setCaretVisible(false);
    midiMessagesBox.setPopupMenuEnabled(true);
    midiMessagesBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0x32ffffff));
    midiMessagesBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(0x1c000000));
    midiMessagesBox.setColour(juce::TextEditor::shadowColourId, juce::Colour(0x16000000));

    // Make sure you set the size of the component after
    // you add any child components.
    setSize(1200, 400);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired(juce::RuntimePermissions::recordAudio) && !juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
                                          [&](bool granted) { setAudioChannels(0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels(0, 2);
    }
}

MainComponent::~MainComponent()
{
    shutdownAudio();

    for (auto s : mSliders)
    {
        delete s;
    }
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    DBG("[MAINCOMPONENT] Sample rate " << sampleRate);
    DBG("[MAINCOMPONENT] Buffer Size " << samplesPerBlockExpected);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill)
{
    if (mAutoencoder)
        mAutoencoder->getNextAudioBlock(bufferToFill);
    else
        bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    openButton.setBounds(10, 10, getWidth() / 12, 20);
    xMaxSlider.setBounds(0, 60, 100, 100);
    sClipSlider.setBounds(100, 60, 100, 100);

    juce::Rectangle<int> layoutArea{240, 5, 600, 190};
    auto sliderArea = layoutArea.removeFromTop(320);

    for (int i = 0; i < n_sliders; ++i)
    {
        mSliders[i]->setBounds(sliderArea.removeFromLeft(70));
    }

    auto area = getLocalBounds();
    midiInputList.setBounds(area.removeFromTop(36).removeFromRight(getWidth() / 4).reduced(8));
    midiMessagesBox.setBounds(area.removeFromBottom(getHeight() / 2));
}

void MainComponent::openButtonClicked()
{
    juce::FileChooser chooser("Select a file to load...", {}, "*.*");

    if (chooser.browseForFileToOpen())
    {
        juce::ScopedLock lock(deviceManager.getAudioCallbackLock());

        auto file = chooser.getResult();
        DBG("[MAIN_COMPONENT] Chosen file: " + file.getFullPathName().toStdString());
        mAutoencoder = std::make_unique<Autoencoder>(file.getFullPathName().toStdString());

        xMaxSlider.setValue(0);
        sClipSlider.setValue(-100);

        for (auto s : mSliders)
        {
            s->setValue(0.0f);
        }
    }
}

void MainComponent::setMidiInput(int index)
{
    auto list = juce::MidiInput::getAvailableDevices();

    deviceManager.removeMidiInputDeviceCallback(list[lastInputIndex].identifier, this);

    auto newInput = list[index];

    if (!deviceManager.isMidiInputDeviceEnabled(newInput.identifier))
        deviceManager.setMidiInputDeviceEnabled(newInput.identifier, true);

    deviceManager.addMidiInputDeviceCallback(newInput.identifier, this);
    midiInputList.setSelectedId(index + 1, juce::dontSendNotification);

    lastInputIndex = index;
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message)
{
    const juce::ScopedValueSetter<bool> scopedInputFlag(isAddingFromMidiInput, true);
    int ccnum;
    if (message.isController())
        ccnum = message.getControllerNumber();
    if ((ccnum >= slider_ccnum) && (ccnum <= slider_ccnum + n_sliders))
    {
        const juce::MessageManagerLock mmLock;

        float mi = mSliders[ccnum - slider_ccnum]->getMinimum();
        float ma = mSliders[ccnum - slider_ccnum]->getMaximum();
        mSliders[ccnum - slider_ccnum]->setValue(linearmap(message.getControllerValue(), 0, 127.0, mi, ma));
    }

#if JUCE_DEBUG
    postMessageToList(message, source->getName());
#endif
}

void MainComponent::postMessageToList(const juce::MidiMessage &message, const juce::String &source)
{
    (new IncomingMessageCallback(this, message, source))->post();
}

void MainComponent::addMessageToList(const juce::MidiMessage &message, const juce::String &source)
{
    auto time = message.getTimeStamp() - startTime;

    auto hours = ((int)(time / 3600.0)) % 24;
    auto minutes = ((int)(time / 60.0)) % 60;
    auto seconds = ((int)time) % 60;
    auto millis = ((int)(time * 1000.0)) % 1000;

    auto timecode = juce::String::formatted("%02d:%02d:%02d.%03d",
                                            hours,
                                            minutes,
                                            seconds,
                                            millis);

    auto description = getMidiMessageDescription(message);

    juce::String midiMessageString(timecode + "  -  " + description + " (" + source + ")"); // [7]
    logMessage(midiMessageString);
}

void MainComponent::logMessage(const juce::String &m)
{
    midiMessagesBox.moveCaretToEnd();
    midiMessagesBox.insertTextAtCaret(m + juce::newLine);
}
