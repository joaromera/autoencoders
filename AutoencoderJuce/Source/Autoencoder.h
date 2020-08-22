/*
  ==============================================================================

    Autoencoder.h
    Created: 18 Aug 2020 11:29:03pm
    Author:  Joaquin Romera

  ==============================================================================
*/

#pragma once

#include <fdeep/fdeep.hpp>
#include <JuceHeader.h>
#include <memory>
#include <string>
#include <vector>

class Autoencoder
{
public:
    Autoencoder(const std::string& path)
    {
        DBG("[AUTOENCODER] Constructing with " + path);
        mAutoencoder = std::make_unique<fdeep::model>(fdeep::load_model(path));
    }

    Autoencoder() = delete;
    Autoencoder(const Autoencoder&) = delete;
    Autoencoder& operator=(const Autoencoder&) = delete;

    ~Autoencoder()
    {
        DBG("[AUTOENCODER] Destroying...");	
    }

    void setHopLength(double newValue)
    {
        DBG("[AUTOENCODER] Hop Length: " << newValue);
        mHopLength = newValue;
    }

    void setWindowLength(double newValue)
    {
        DBG("[AUTOENCODER] Window Length: " << newValue);
        mWindowLength = newValue;
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        bufferToFill.clearActiveBufferRegion();
    }

private:
    const size_t mDepth {441};
    fdeep::tensor_shape mTensorShapeDepth {441};
    fdeep::tensors mTensors;
    std::unique_ptr<fdeep::model> mEncoder;
    std::unique_ptr<fdeep::model> mDecoder;
    std::unique_ptr<fdeep::model> mAutoencoder;
    fdeep::tensors mPredictionResult {};

    double mHopLength {10};
    double mWindowLength {10};
};
