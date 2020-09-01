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
    explicit Autoencoder(const std::string& path)
    {
        DBG("[AUTOENCODER] Constructing with " << path);
        mAutoencoder = std::make_unique<fdeep::model>(fdeep::load_model(path));

        mDepth = mAutoencoder->get_input_shapes()[0].depth_.unsafe_get_just();
        DBG("[AUTOENCODER] INPUT DEPTH: " << mDepth);
        DBG("[AUTOENCODER] OUTPUT DEPTH: " << mAutoencoder->get_output_shapes()[0].depth_.unsafe_get_just());

        mInput = std::vector<float>(mDepth, 0.0f);
        mTensorShapeDepth = std::make_unique<fdeep::tensor_shape>(mDepth);
        mTensors = {fdeep::tensor(*mTensorShapeDepth, mInput)};
        mPredictionResult = mAutoencoder->predict(mTensors);
        DBG("[AUTOENCODER] Prediction results: " << fdeep::show_tensors(mPredictionResult));
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
    std::unique_ptr<fdeep::model> mAutoencoder;
    std::size_t mDepth;
    std::vector<float> mInput;
    std::unique_ptr<fdeep::tensor_shape> mTensorShapeDepth;
    fdeep::tensors mTensors;
    fdeep::tensors mPredictionResult;

    double mHopLength {10};
    double mWindowLength {10};
};
