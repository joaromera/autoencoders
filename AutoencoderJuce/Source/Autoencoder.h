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
#include <utility>
#include <vector>

class Autoencoder
{
    struct parameters;
    struct slider;

public:

    explicit Autoencoder(const std::string& pModel, const parameters &pParams)
        : mAutoencoder(fdeep::read_model_from_string(pModel, false, nullptr))
        , mInput(mAutoencoder.get_input_shapes()[0].depth_.unsafe_get_just(), 0.0f)
        , mTensors { fdeep::tensor(fdeep::tensor_shape{mInput.size()}, mInput) }
        , mParams(pParams)
        , rfft_size(mAutoencoder.get_output_shapes()[0].depth_.unsafe_get_just())
        , mask(mParams.win_length - 1)
        , index(0)
        , idx_proc(0)
        , mAudio(mParams.win_length, 0.0f)
        , random {}
        , mFFT {12}
    {
        DBG("[AUTOENCODER] INPUT DEPTH: " << mInput.size());
        DBG("[AUTOENCODER] OUTPUT DEPTH: " << mAutoencoder.get_output_shapes()[0].depth_.unsafe_get_just());

        // TODO fix how to initialize values to avoid pops in audio when loading
        mParams.xMax = 0;
        mParams.sClip = -100;
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

        parameters params;
        params.win_length = settings["parameters"]["win_length"];
        params.sClip = std::stof(settings["parameters"]["sClip"].get<std::string>());
        params.xMax = std::stof(settings["parameters"]["xMax"].get<std::string>());
        for (const auto &i : settings["parameters"]["zRange"])
        {
            params.zRange.emplace_back(
                std::stof(i["min"].get<std::string>()),
                std::stof(i["max"].get<std::string>())
            );
        }

        return std::make_unique<Autoencoder>(settings["model"].dump(), params);
    }

    void setInputLayers(const size_t pos, const float newValue)
    {
        DBG("[AUTOENCODER] slider: " << pos << " new value: " << newValue);
        mTensors.at(0).set(fdeep::tensor_pos {pos}, newValue);
    }

    size_t getInputDepth() const
    {
        return mInput.size();
    }

    std::pair<float, float> getSlider(const size_t pos)
    {
        return { mParams.zRange[pos].min, mParams.zRange[pos].max };
    }

    void setXMax(const float newValue)
    {
        DBG("[AUTOENCODER] xMax Length: " << newValue);
        mParams.xMax = newValue;
    }

    void setSClip(const float newValue)
    {
        DBG("[AUTOENCODER] sClip Length: " << newValue);
        mParams.sClip = newValue;
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        calculate(bufferToFill.numSamples);

        for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
        {
            bufferToFill.buffer->setSample(0, sample, mAudio[index + sample]);
            bufferToFill.buffer->setSample(1, sample, mAudio[index + sample]);
            mAudio[index + sample] = 0;
        }

        index += bufferToFill.numSamples;
        index &= mask;
    }

    void calculate(const int bufferToFillSize)
    {
        const auto predictionResult = mAutoencoder.predict(mTensors)[0].as_vector();

        for (int i = 0; i < rfft_size; ++i)
        {
            const float power = ((predictionResult->at(i) * mParams.xMax) + mParams.sClip) / 10;
            const float y_aux = std::sqrt(std::pow(10, power));
            const float phase = random.nextFloat() * juce::MathConstants<float>::twoPi;
            fft_array[2 * i] = y_aux * std::cos(phase);
            fft_array[(2 * i) + 1] = y_aux * std::sin(phase);
        }

        mFFT.performRealOnlyInverseTransform(fft_array);

        for (int i = 0; i < mParams.win_length; ++i)
        {
            const float multiplier = 0.5f * (
                1 - std::cos(juce::MathConstants<float>::twoPi * i / (mParams.win_length - 1))
            );
            const float sample = fft_array[i] * multiplier;
            const int idx = (idx_proc + i) & mask;
            mAudio[idx] += sample;
        }

        idx_proc += bufferToFillSize;
        idx_proc &= mask;
    }

private:

    struct slider
    {
        slider(float pMin, float pMax) : min(pMin), max(pMax) {}

        float min;
        float max;
    };

    struct parameters
    {
        float xMax;
        float sClip;
        unsigned win_length;
        std::vector<slider> zRange;
    };

    const fdeep::model mAutoencoder;
    std::vector<float> mInput;
    fdeep::tensors mTensors;

    parameters mParams;
    const unsigned rfft_size;

    float fft_array[4096*2] = {};
    const int mask;
    int index;
    int idx_proc;
    std::vector<float> mAudio;

    juce::Random random;
    juce::dsp::FFT mFFT;
};
