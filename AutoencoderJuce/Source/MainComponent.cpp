#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : adsc(deviceManager, 2, 2, 2, 2, false, false, false, false)
    , startTime (juce::Time::getMillisecondCounterHiRes() * 0.001)
{
    addAndMakeVisible(adsc);
    adsc.setBounds(300, 25, 300, 300);

    // Open button
    openButton.setButtonText ("Open...");
    openButton.onClick = [this] { openButtonClicked(); };
    addAndMakeVisible (&openButton);
    
    // Hop Length Slider
    hopLengthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    hopLengthSlider.setRange(0,100,1);
    hopLengthSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 90, 0);
    hopLengthSlider.setPopupDisplayEnabled(true, false, this);
    hopLengthSlider.setTextValueSuffix(" Hop Length");
    hopLengthSlider.setValue(1.0);
    hopLengthSlider.onValueChange = [this] {
        DBG("[MAINCOMPONENT] HopLengthSlider: new value " << hopLengthSlider.getValue());
        if (mAutoencoder)
            mAutoencoder->setHopLength(hopLengthSlider.getValue());
    };
    addAndMakeVisible (&hopLengthSlider);
    
    // Window Length Slider
    windowLengthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    windowLengthSlider.setRange(0,100,1);
    windowLengthSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 90, 0);
    windowLengthSlider.setPopupDisplayEnabled(true, false, this);
    windowLengthSlider.setTextValueSuffix(" Window Length");
    windowLengthSlider.setValue(1.0);
    windowLengthSlider.onValueChange = [this] {
        DBG("[MAINCOMPONENT] WindowLengthSlider: new value " << windowLengthSlider.getValue());
        if (mAutoencoder)
            mAutoencoder->setWindowLength(windowLengthSlider.getValue());
    };
    addAndMakeVisible (&windowLengthSlider);
    
    // MIDI INPUT
    addAndMakeVisible (midiInputList);
    midiInputList.setTextWhenNoChoicesAvailable ("No MIDI Inputs Enabled");
    auto midiInputs = juce::MidiInput::getAvailableDevices();

    juce::StringArray midiInputNames;

    for (const auto& input : midiInputs) midiInputNames.add (input.name);

    midiInputList.addItemList (midiInputNames, 1);
    midiInputList.onChange = [this] { setMidiInput (midiInputList.getSelectedItemIndex()); };

    // find the first enabled device and use that by default
    for (const auto& input : midiInputs)
    {
        if (deviceManager.isMidiInputDeviceEnabled (input.identifier))
        {
            setMidiInput (midiInputs.indexOf (input));
            break;
        }
    }

    // if no enabled devices were found just use the first one in the list
    if (midiInputList.getSelectedId() == 0) setMidiInput (0);

    addAndMakeVisible (midiMessagesBox);
    midiMessagesBox.setMultiLine (true);
    midiMessagesBox.setReturnKeyStartsNewLine (true);
    midiMessagesBox.setReadOnly (true);
    midiMessagesBox.setScrollbarsShown (true);
    midiMessagesBox.setCaretVisible (false);
    midiMessagesBox.setPopupMenuEnabled (true);
    midiMessagesBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x32ffffff));
    midiMessagesBox.setColour (juce::TextEditor::outlineColourId, juce::Colour (0x1c000000));
    midiMessagesBox.setColour (juce::TextEditor::shadowColourId, juce::Colour (0x16000000));

    // Make sure you set the size of the component after
    // you add any child components.
    setSize (600, 400);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (0, 2);
    }
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    DBG("[MAINCOMPONENT] Sample rate " << sampleRate);
    DBG("[MAINCOMPONENT] Buffer Size " << samplesPerBlockExpected);
    //auto db = deviceManager.getAudioDeviceSetup();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
//    bufferToFill.clearActiveBufferRegion();

    if (mAutoencoder) mAutoencoder->getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    openButton.setBounds(10, 10, getWidth() / 3, 20);
    hopLengthSlider.setBounds(0, 30, (getWidth() - 20) / 4, (getWidth() - 20) / 4);
    windowLengthSlider.setBounds((getWidth() - 20) / 4, 30, (getWidth() - 20) / 4, (getWidth() - 20) / 4);
    
    auto area = getLocalBounds();

    midiInputList    .setBounds (area.removeFromTop(36).removeFromRight(getWidth() / 2).reduced(8));
    midiMessagesBox  .setBounds (area.removeFromBottom (getHeight()/2));
}

void MainComponent::openButtonClicked()
{
    juce::FileChooser chooser ("Select a H5 file to load...", {}, "*.h5");

    if (chooser.browseForFileToOpen())
    {
        juce::ScopedLock lock(deviceManager.getAudioCallbackLock());

        auto file = chooser.getResult();
        DBG("[MAIN_COMPONENT] Chosen file: " + file.getFullPathName().toStdString());
        mAutoencoder = std::make_unique<Autoencoder>(file.getFullPathName().toStdString());
    }
}

void MainComponent::setMidiInput (int index)
{
    auto list = juce::MidiInput::getAvailableDevices();

    deviceManager.removeMidiInputDeviceCallback(list[lastInputIndex].identifier, this);

    auto newInput = list[index];

    if (! deviceManager.isMidiInputDeviceEnabled (newInput.identifier))
        deviceManager.setMidiInputDeviceEnabled (newInput.identifier, true);

    deviceManager.addMidiInputDeviceCallback (newInput.identifier, this);
    midiInputList.setSelectedId (index + 1, juce::dontSendNotification);

    lastInputIndex = index;
}

void MainComponent::handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message)
{
    const juce::ScopedValueSetter<bool> scopedInputFlag (isAddingFromMidiInput, true);
    postMessageToList(message, source->getName());
}

void MainComponent::postMessageToList (const juce::MidiMessage& message, const juce::String& source)
{
    (new IncomingMessageCallback (this, message, source))->post();
}

void MainComponent::addMessageToList(const juce::MidiMessage& message, const juce::String& source)
{
    auto time = message.getTimeStamp() - startTime;

    auto hours   = ((int) (time / 3600.0)) % 24;
    auto minutes = ((int) (time / 60.0)) % 60;
    auto seconds = ((int) time) % 60;
    auto millis  = ((int) (time * 1000.0)) % 1000;

    auto timecode = juce::String::formatted ("%02d:%02d:%02d.%03d",
                                             hours,
                                             minutes,
                                             seconds,
                                             millis);

    auto description = getMidiMessageDescription (message);

    juce::String midiMessageString (timecode + "  -  " + description + " (" + source + ")"); // [7]
    logMessage (midiMessageString);
}

void MainComponent::logMessage (const juce::String& m)
{
    midiMessagesBox.moveCaretToEnd();
    midiMessagesBox.insertTextAtCaret (m + juce::newLine);
}
