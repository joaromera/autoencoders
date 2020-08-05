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
#include <exception>

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
    , mEncoder(nullptr)
    , mDecoder(nullptr)
    , mAutoencoder(nullptr)
    , mPredictionResult()
{
    mTensors.push_back(fdeep::tensor(mTensorShapeDepth, mInput));
    try {
//        juce::File path ("/Users/jromera/Documents/UNQ/tesis/autoencoders/AutoencoderJuce/autoencoder.h5");
//        const std::string pathStr = path.loadFileAsString().toStdString();
        mAutoencoder = new fdeep::model (fdeep::load_model("/Users/jromera/Documents/UNQ/tesis/autoencoders/AutoencoderJuce/autoencoder.h5"));
    } catch (std::exception &e) {
        DBG(e.what());
        DBG("OUCH");
    }
}

AutoencoderJuceAudioProcessor::~AutoencoderJuceAudioProcessor()
{
    if (mAutoencoder) delete mAutoencoder;
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

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto* channelLeft  = buffer.getWritePointer (0);
    auto* channelRight = buffer.getWritePointer (1);
    
    const fdeep::tensors decoderOutput = mAutoencoder->predict(mTensors);
    
    std::vector<dsp::Complex<float>> in(decoderOutput.size());
    std::vector<dsp::Complex<float>> out(decoderOutput.size());
    
    for (const auto &wa : *(decoderOutput[0].as_vector()))
    {
        in.emplace_back(wa);
        out.push_back(0);
    }
    
    for (int sample = 0; sample < out.size(); ++sample)
    {
        mFFT.perform(&in[sample], &out[sample], true);
    }
    
//    for (size_t sample = 0; sample < decoderOutput[0].as_vector()->size(); ++sample)
//    {
//        in[sample] = decoderOutput[0].as_vector()->operator[](sample);
//    }
//    
//    mFFT.performRealOnlyInverseTransform(in);
    
    for (int sample = 0; sample < buffer.getNumSamples() && sample < out.size(); ++sample)
    {
        channelLeft[sample] = out[sample].real();
        channelRight[sample] = out[sample].real();
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
