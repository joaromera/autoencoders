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
#include <string>
#include <vector>

class Autoencoder
{
public:
    Autoencoder(const std::string& path)
    : mFFT(512)
    , mDepth(441)
    , mInput(mDepth, 1.0f)
    , mTensorShapeDepth(mDepth)
    , mEncoder(nullptr)
    , mDecoder(nullptr)
    , mAutoencoder(nullptr)
    , mPredictionResult()
    {
        mTensors.push_back(fdeep::tensor(mTensorShapeDepth, mInput));
        mAutoencoder = new fdeep::model (fdeep::load_model(path));
    }
    
    Autoencoder() = delete;
    Autoencoder(const Autoencoder&) = delete;
    Autoencoder& operator=(const Autoencoder&) = delete;

    ~Autoencoder()
    {
        if (mAutoencoder) delete mAutoencoder;
    }
    
private:
    const juce::dsp::FFT mFFT;
    const size_t mDepth;
    
    std::vector<float> mInput;
    
    fdeep::tensor_shape mTensorShapeDepth;
    fdeep::tensors mTensors;
    fdeep::model* mEncoder;
    fdeep::model* mDecoder;
    fdeep::model* mAutoencoder;
    float in[1024];
    fdeep::tensors mPredictionResult;
};
