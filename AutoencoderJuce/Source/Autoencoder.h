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
    {
        DBG("[AUTOENCODER] Constructing with " << path);
        mAutoencoder = std::make_unique<const fdeep::model>(fdeep::load_model(path));

        mDepth = mAutoencoder->get_input_shapes()[0].depth_.unsafe_get_just();
        DBG("[AUTOENCODER] INPUT DEPTH: " << mDepth);
        DBG("[AUTOENCODER] OUTPUT DEPTH: " << mAutoencoder->get_output_shapes()[0].depth_.unsafe_get_just());

        mInput = std::vector<float>(mDepth, 0.0f);
        mTensorShapeDepth = std::make_unique<fdeep::tensor_shape>(mDepth);
        mTensors = { fdeep::tensor(*mTensorShapeDepth, mInput) };

//        mPredictionResult = mAutoencoder->predict(mTensors);
//        DBG("[AUTOENCODER] Prediction results: " << fdeep::show_tensors(mPredictionResult));

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
        if (!ready)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        calculate();

        // Sine wave test
        if (mHopLength < 50)
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
//        for (int i = 0; i < 8; ++i) Z[i] = random.nextFloat();

//        const fdeep::tensor_shape depth (Z.size());
//        const fdeep::tensors tensors = { fdeep::tensor(depth, Z) };
//        auto Y_predict = mAutoencoder->predict_stateful(tensors)[0].as_vector();
        auto Y_predict = mAutoencoder->predict(mTensors)[0].as_vector();

        for (unsigned i = 0; i < rfft_size; ++i)
        {
            const float power = ((Y_predict->operator[](i) * xMax) + sClip) / 10;
            const float y_aux = std::sqrt(std::pow(10, power));
            const float phase = random.nextFloat() * 2 * PI;
            ca[i].x = y_aux * std::cos(phase);
            ca[i].y = y_aux * std::sin(phase);
        }

        alglib::fftr1dinv(ca, audio_m);

        for (int i = 0; i < win_length; ++i)
        {
            const float multiplier = 0.5f * (1 - std::cos(2 * PI * i / (win_length - 1)));
            audio_m[i] = multiplier * audio_m[i];
            int idx = idx_proc + i;
            idx &= mask;
            mAudio[idx] += audio_m[i];
        }
        idx_proc += hop_length;
        idx_proc &= mask;
    }

private:
    std::unique_ptr<const fdeep::model> mAutoencoder;
    std::size_t mDepth;
    std::vector<float> mInput;
    std::unique_ptr<fdeep::tensor_shape> mTensorShapeDepth;
    fdeep::tensors mTensors;
    fdeep::tensors mPredictionResult;

    const double PI = std::acos(-1);
    const double xMax = 96;
    const double sClip = -60;
    const unsigned win_length = 2048;
    const unsigned rfft_size = (win_length / 2) + 1;
    const unsigned N = 1;
    const unsigned hop_length_ms = 10;
    const unsigned sr = 22050;
    const unsigned hop_length = 512;

    alglib::complex_1d_array ca;
    alglib::real_1d_array audio_m;
    const int mask = win_length - 1;
    int index = 0;
    int idx_proc = 0;
    std::vector<float> mAudio = std::vector<float>(win_length, 0.0f);
//    std::vector<float> Z = std::vector<float>(8, .8f);
    std::vector<float> Z = {1,0,0,0,1,0,0,0};
    double mHopLength {10};
    double mWindowLength {10};

    juce::Random random{};

    // sine wave test
    float m_amplitude = 0.01;
    float m_frequency = 512;
    float m_phase = 0.0;
    float m_time = 0.0;
    float m_deltaTime = 1.0 / 44100;

    std::atomic<bool> ready = false;
};
