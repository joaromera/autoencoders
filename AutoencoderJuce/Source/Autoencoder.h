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
    explicit Autoencoder(const std::string& path)
        : mAutoencoder(fdeep::load_model(path))
        , mDepth(mAutoencoder.get_input_shapes()[0].depth_.unsafe_get_just())
        , mInput(mDepth, 0.0f)
        , mTensorShapeDepth(mDepth)
        , mTensors { fdeep::tensor(mTensorShapeDepth, mInput) }
    {
        DBG("[AUTOENCODER] Constructing with " << path);
        DBG("[AUTOENCODER] INPUT DEPTH: " << mDepth);
        DBG("[AUTOENCODER] OUTPUT DEPTH: " << mAutoencoder.get_output_shapes()[0].depth_.unsafe_get_just());
    }

    Autoencoder() = delete;
    Autoencoder(const Autoencoder&) = delete;
    Autoencoder& operator=(const Autoencoder&) = delete;

    ~Autoencoder()
    {
        DBG("[AUTOENCODER] Destroying...");
    }

    void setInputLayers(const size_t pos, const float newValue)
    {
        DBG("[AUTOENCODER] slider: " << pos << " new value: " << newValue);
        mTensors.at(0).set(fdeep::tensor_pos {pos}, newValue);
    }

    size_t getInputDepth() const
    {
        return mDepth;
    }

    void setXMax(const float newValue)
    {
        DBG("[AUTOENCODER] xMax Length: " << newValue);
        xMax = newValue;
    }

    void setSClip(const float newValue)
    {
        DBG("[AUTOENCODER] sClip Length: " << newValue);
        sClip = newValue;
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        calculate();

        for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
        {
            bufferToFill.buffer->setSample(0, sample, mAudio[index + sample]);
            bufferToFill.buffer->setSample(1, sample, mAudio[index + sample]);
            mAudio[index + sample] = 0;
        }

        index += bufferToFill.numSamples;
        index &= mask;
    }

    void calculate()
    {
        const auto predictionResult = mAutoencoder.predict(mTensors)[0].as_vector();

        for (int i = 0; i < rfft_size; ++i)
        {
            const float power = ((predictionResult->at(i) * xMax) + sClip) / 10;
            const float y_aux = std::sqrt(std::pow(10, power));
            const float phase = random.nextFloat() * juce::MathConstants<float>::twoPi;
            fft_array[i] = y_aux * std::cos(phase);
        }

        mFFT.performRealOnlyInverseTransform(fft_array);

        for (int i = 0; i < win_length; ++i)
        {
            const float multiplier = 0.5f * (
                1 - std::cos(juce::MathConstants<float>::twoPi * i / (win_length - 1))
            );
            fft_array[i] *= multiplier;
            const int idx = (idx_proc + i) & mask;
            mAudio[idx] += fft_array[i];
        }

        idx_proc += hop_length;
        idx_proc &= mask;
    }

private:

    const fdeep::model mAutoencoder;
    const std::size_t mDepth;
    std::vector<float> mInput;
    const fdeep::tensor_shape mTensorShapeDepth;
    fdeep::tensors mTensors;

    float xMax = 0;
    float sClip = -100;
    const unsigned win_length = 2048;
    const unsigned rfft_size = (win_length / 2) + 1;
    const unsigned hop_length = 512;

    float fft_array[4096] = {};
    const int mask = win_length - 1;
    int index = 0;
    int idx_proc = 0;
    std::vector<float> mAudio = std::vector<float>(win_length, 0.0f);

    juce::Random random {};
    juce::dsp::FFT mFFT {11};
};
