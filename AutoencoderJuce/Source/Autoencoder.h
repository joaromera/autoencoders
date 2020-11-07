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
    struct Parameters;
    struct SliderMinMax;

public:

    explicit Autoencoder(const std::string& pModel, const Parameters &pParams)
        : mAutoencoder(fdeep::read_model_from_string(pModel, false, nullptr))
        , mInput(mAutoencoder.get_input_shapes()[0].depth_.unsafe_get_just(), 0.0f)
        , mTensors { fdeep::tensor(fdeep::tensor_shape{mInput.size()}, mInput) }
        , mParams(pParams)
        , rfftSize(mAutoencoder.get_output_shapes()[0].depth_.unsafe_get_just())
        , fftArray {}
        , mask(mParams.winLength - 1)
        , index(0)
        , idxProc(0)
        , mAudio(mParams.winLength, 0.0f)
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

        Parameters params = {
            std::stof(settings["parameters"]["xMax"].get<std::string>()),
            std::stof(settings["parameters"]["sClip"].get<std::string>()),
            settings["parameters"]["win_length"],
            settings["parameters"]["zRange"]
        };

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

        for (int i = 0; i < rfftSize; ++i)
        {
            const float power = ((predictionResult->at(i) * mParams.xMax) + mParams.sClip) / 10;
            const float y_aux = std::sqrt(std::pow(10, power));
            const float phase = random.nextFloat() * juce::MathConstants<float>::twoPi;
            fftArray[2 * i] = y_aux * std::cos(phase);
            fftArray[(2 * i) + 1] = y_aux * std::sin(phase);
        }

        mFFT.performRealOnlyInverseTransform(fftArray);

        for (int i = 0; i < mParams.winLength; ++i)
        {
            const float multiplier = 0.5f * (
                1 - std::cos(juce::MathConstants<float>::twoPi * i / (mParams.winLength - 1))
            );
            const float sample = fftArray[i] * multiplier;
            const int idx = (idxProc + i) & mask;
            mAudio[idx] += sample;
        }

        idxProc += bufferToFillSize;
        idxProc &= mask;
    }

private:

    struct SliderMinMax
    {
        float min;
        float max;
    };

    struct Parameters
    {
        Parameters(float pXMax, float pSClip, const unsigned int pWinLength, const nlohmann::json &PZRange)
            : xMax(pXMax)
            , sClip(pSClip)
            , winLength(pWinLength)
        {
            for (const auto &r : PZRange)
                zRange.push_back({
                    std::stof(r["min"].get<std::string>()),
                    std::stof(r["max"].get<std::string>())
                });
        }

        float xMax;
        float sClip;
        const unsigned winLength;
        std::vector<SliderMinMax> zRange;
    };

    const fdeep::model mAutoencoder;
    std::vector<float> mInput;
    fdeep::tensors mTensors;

    Parameters mParams;
    const unsigned rfftSize;

    float fftArray[4096 * 2];
    const int mask;
    int index;
    int idxProc;
    std::vector<float> mAudio;

    juce::Random random;
    juce::dsp::FFT mFFT;
};
