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
#include "fasttransforms.h"

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

        mTensors.at(0).set(fdeep::tensor_pos{0}, 5.0f);

        ca.setlength(win_length);
        audio_m.setlength(win_length);

        ready = true;
    }

    Autoencoder() = delete;
    Autoencoder(const Autoencoder&) = delete;
    Autoencoder& operator=(const Autoencoder&) = delete;

    ~Autoencoder()
    {
        ready = false;
        DBG("[AUTOENCODER] Destroying...");
    }

    void setInputLayers(size_t pos, double newValue)
    {
        DBG("[AUTOENCODER] slider: " << pos << " new value: " << newValue);
        mTensors.at(0).set(fdeep::tensor_pos {pos}, newValue);
    }

    void setXMax(double newValue)
    {
        DBG("[AUTOENCODER] Hop Length: " << newValue);
        xMax = newValue;
    }

    void setSClip(double newValue)
    {
        DBG("[AUTOENCODER] Window Length: " << newValue);
        sClip = newValue;
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        if (!ready)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        calculate();

        // Sine wave test
        if (false)
        {
            if (m_time >= std::numeric_limits<float>::max()) {
                m_time = 0.0;
            }

            for (int sample = 0; sample < bufferToFill.numSamples; ++sample) {
                float value = m_amplitude * std::sin((2 * PI * m_frequency * m_time) + m_phase);
                mAudio[sample] = value;
                m_time += m_deltaTime;
            }

            for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
            {
                bufferToFill.buffer->setSample(0, sample, mAudio[sample]);
                bufferToFill.buffer->setSample(1, sample, mAudio[sample]);
            }
        }
        else
        {
            for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
            {
                bufferToFill.buffer->setSample(0, sample, mAudio[index + sample]);
                bufferToFill.buffer->setSample(1, sample, mAudio[index + sample]);
                mAudio[index + sample] = 0;
            }
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
            const float phase = random.nextFloat() * 2 * PI;
            ca[i].x = y_aux * std::cos(phase);
            ca[i].y = y_aux * std::sin(phase);
        }

        alglib::fftr1dinv(ca, audio_m);

        for (int i = 0; i < win_length; ++i)
        {
            const float multiplier = 0.5f * (1 - std::cos(2 * PI * i / (win_length - 1)));
            audio_m[i] *= multiplier;
            const int idx = (idx_proc + i) & mask;
            mAudio[idx] += audio_m[i];
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

    const double PI = std::acos(-1);
    double xMax = 96;
    double sClip = -60;
    const unsigned win_length = 2048;
    const unsigned rfft_size = (win_length / 2) + 1;
    const unsigned sr = 22050;
    const unsigned hop_length = 512;

    alglib::complex_1d_array ca;
    alglib::real_1d_array audio_m;
    const int mask = win_length - 1;
    int index = 0;
    int idx_proc = 0;
    std::vector<float> mAudio = std::vector<float>(win_length, 0.0f);

    juce::Random random{};

    // sine wave test
    const float m_amplitude = 0.01;
    const float m_frequency = 512;
    const float m_phase = 0.0;
    float m_time = 0.0;
    float m_deltaTime = 1.0 / 44100;

    std::atomic<bool> ready = false;
};
