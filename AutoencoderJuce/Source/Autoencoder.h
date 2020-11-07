/*
  ==============================================================================

    Autoencoder.h
    Created: 18 Aug 2020 11:29:03pm
    Author:  Joaquin Romera

  ==============================================================================
*/

#pragma once

#include <complex>
#include <fdeep/fdeep.hpp>
#include <fstream>
#include <JuceHeader.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

class Autoencoder
{
public:
    explicit Autoencoder(const std::string& pModel, const float pXMax, const float pSClip, const unsigned pWinLength, const unsigned pHopLength)
        : mAutoencoder(fdeep::read_model_from_string(pModel))
        , mDepth(mAutoencoder.get_input_shapes()[0].depth_.unsafe_get_just())
        , mInput(mDepth, 0.0f)
        , mTensorShapeDepth(mDepth)
        , mTensors { fdeep::tensor(mTensorShapeDepth, mInput) }
        , xMax(0) // TODO fix this
        , sClip(-100) // TODO fix this
        , win_length(pWinLength)
        , rfft_size(mAutoencoder.get_output_shapes()[0].depth_.unsafe_get_just())
        , hop_length(pHopLength)
        , mask(win_length - 1)
        , index(0)
        , idx_proc(0)
        , mAudio(win_length, 0.0f)
        , random {}
        , mFFT {11}
    {
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

    static std::unique_ptr<Autoencoder> MakeAutoencoder(const std::string& path)
    {
        std::ifstream ifs (path);
        nlohmann::json settings;
        ifs >> settings;
        return std::make_unique<Autoencoder>(
            settings.at("model").dump(),
            std::stof(settings.at("xMax").get<std::string>()),
            settings.at("sClip"),
            settings.at("win_length"),
            settings.at("hop_length")
        );
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
            complex_fft_array[i] = std::complex<float>(y_aux * std::cos(phase), y_aux * std::sin(phase));
        }

        mFFT.perform(complex_fft_array, real_fft_array, true);

        for (int i = 0; i < win_length; ++i)
        {
            const float multiplier = 0.5f * (
                1 - std::cos(juce::MathConstants<float>::twoPi * i / (win_length - 1))
            );
            const float sample = real_fft_array[i].real() * multiplier;
            const int idx = (idx_proc + i) & mask;
            mAudio[idx] += sample;
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

    float xMax;
    float sClip;
    const unsigned win_length;
    const unsigned rfft_size;
    const unsigned hop_length;

    std::complex<float> complex_fft_array[4096] = {};
    std::complex<float> real_fft_array[4096] = {};
    const int mask;
    int index;
    int idx_proc;
    std::vector<float> mAudio;

    juce::Random random;
    juce::dsp::FFT mFFT;
};
