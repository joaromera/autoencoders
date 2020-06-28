/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <fdeep/fdeep.hpp>
#include <memory>

//==============================================================================
AutoencoderJuceAudioProcessor::AutoencoderJuceAudioProcessor()
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
    // @TODO: verificar si mFFT, mDepth, mInput y mTensorShapeDepth realmente tienen que ser const
    // o si pueden/deberían ser parametrizables
    , mFFT(512)
    , mDepth(441)
    , mInput(mDepth, 1.0f)
    , mTensorShapeDepth(mDepth)
    , mTensors { fdeep::tensor(mTensorShapeDepth, mInput) }
    , mEncoder(nullptr)
    , mDecoder(nullptr)
    , mAutoencoder(nullptr)
    , mPredictionResult()
{
    // @TODO: al construir una nueva instancia ver qué modelo cargar (identidad?)
    mAutoencoder = new fdeep::model (fdeep::load_model("../autoencoder.h5"));
}

AutoencoderJuceAudioProcessor::~AutoencoderJuceAudioProcessor()
{
    delete mAutoencoder;
}

//==============================================================================
const String AutoencoderJuceAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AutoencoderJuceAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AutoencoderJuceAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AutoencoderJuceAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AutoencoderJuceAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AutoencoderJuceAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AutoencoderJuceAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AutoencoderJuceAudioProcessor::setCurrentProgram (int index)
{
}

const String AutoencoderJuceAudioProcessor::getProgramName (int index)
{
    return {};
}

void AutoencoderJuceAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void AutoencoderJuceAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void AutoencoderJuceAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AutoencoderJuceAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void AutoencoderJuceAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto* channelLeft  = buffer.getWritePointer (0);
    auto* channelRight = buffer.getWritePointer (1);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        channelLeft[sample] *= 0.5;
        channelRight[sample] *= 0.5;
    }
}

//==============================================================================
bool AutoencoderJuceAudioProcessor::hasEditor() const
{
    return false;
    // return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* AutoencoderJuceAudioProcessor::createEditor()
{
    return nullptr;
    // return new AutoencoderJuceAudioProcessorEditor (*this);
}

//==============================================================================
void AutoencoderJuceAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void AutoencoderJuceAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutoencoderJuceAudioProcessor();
}
