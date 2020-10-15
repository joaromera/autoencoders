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

class Autoencoder : public juce::Thread
{
public:
    explicit Autoencoder(const std::string& path) : Thread("Prediction Thread")
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
        startThread();
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
        if (!ready) return;

        for (size_t i = 0; i < bufferToFill.numSamples; ++i)
        {
            bufferToFill.buffer->setSample(0, i, mAudio[index]);
            bufferToFill.buffer->setSample(1, i, mAudio[index]);
            mAudio[index] = 0;
            ++index;
            index &= mask;
        }

        ++three;
        if (three == 4)
        {
            three = 0;
            startThread();
        }
    }

    void calculate()
    {
        for (unsigned n = 0; n < N; ++n)
        {
            for (int i = 0; i < 8; ++i)
            {
                Z[i] = random.nextFloat();
            }

            fdeep::tensor_shape depth (Z.size());
            fdeep::tensors tensors = { fdeep::tensor(depth, Z) };
            auto Y_predict = mAutoencoder->predict(tensors);

            ca.setlength(win_length);

            for (unsigned i = 0; i < rfft_size; ++i)
            {
                const float power = ((Y_predict[0].as_vector()->operator[](i) * xMax) + sClip) / 10;
                const float y_aux = std::sqrt(std::pow(10, power));
                const float phase = random.nextFloat() * 2 * PI;
                ca[i].x = y_aux * std::cos(phase);
                ca[i].y = y_aux * std::sin(phase);
            }

            alglib::fftr1dinv(ca, audio_m);

            for (int i = 0; i < win_length; i++)
            {
                double multiplier = 0.5f * (1 - std::cos(2 * PI * i / (win_length - 1)));
                audio_m[i] = multiplier * audio_m[i];
                int idx = idx_proc + (n*hop_length)+i;
                idx &= mask;
                mAudio[idx] += audio_m[i];
            }
        }
        idx_proc += ((N-1) * hop_length) + win_length;
        idx_proc &= mask;
        ready = true;
    }

    void run() override
    {
        calculate();
    }
 
private:
    std::unique_ptr<fdeep::model> mAutoencoder;
    std::size_t mDepth;
    std::vector<float> mInput;
    std::unique_ptr<fdeep::tensor_shape> mTensorShapeDepth;
    fdeep::tensors mTensors;
    fdeep::tensors mPredictionResult;

    const double PI = std::acos(-1);
    const double xMax = 96;
    const double sClip = -60;
    const unsigned win_length = 880;
    const unsigned rfft_size = (win_length / 2) + 1;
    const unsigned N = 15;
    const unsigned hop_length_ms = 10;
    const unsigned sr = 22050;
    const unsigned hop_length = ((float) hop_length_ms / 1000) * sr;

    alglib::complex_1d_array ca;
    alglib::real_1d_array audio_m;
    int three = 0;
    const int mask = 4095;
    int index = 0;
    int idx_proc = 0;
    std::vector<float> mAudio = std::vector<float>(4096, 0.0f);
    std::vector<float> Z = std::vector<float>(8, 0.0f);
    double mHopLength {10};
    double mWindowLength {10};
    bool ready = false;

    juce::Random random{};
};
