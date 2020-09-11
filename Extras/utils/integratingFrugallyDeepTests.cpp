#define CATCH_CONFIG_MAIN
#include "catch.hh"

#include "interp1d.hpp"
#include "fdeep/fdeep.hpp"
#include "AudioFile.h"
#include <Eigen/Core>
#include <unsupported/Eigen/FFT>

class FrugallyDeepIntegration
{
public:
    fdeep::model model = fdeep::load_model("decoder.h5");
};

TEST_CASE_METHOD(FrugallyDeepIntegration, "Integrate with Frugally Deep", "[Calling]")
{
    auto test = inputForPrediction<float, float>(12, 5);
    REQUIRE(test.size() == 60);
    REQUIRE(test[0].size() == 8);

    fdeep::tensor_shape depth (test[0].size());

    std::vector<fdeep::tensors> tensors;
    for (const auto &t : test)
    {
        tensors.push_back({fdeep::tensor(depth, t)});
    }

    const auto result = model.predict_multi(tensors, false);
    REQUIRE(result.size() == 60);
    REQUIRE(result[0][0].depth() == 441);
}

TEST_CASE_METHOD(FrugallyDeepIntegration,
                 "Send prediction results to inputForIFFT",
                 "[Calling]")
{
    auto test = inputForPrediction<float, float>(60, 25);
    fdeep::tensor_shape depth (test[0].size());

    std::vector<fdeep::tensors> tensors;
    for (const auto &t : test)
        tensors.push_back({fdeep::tensor(depth, t)});

    const auto predictionResult = model.predict_multi(tensors, false);

    std::vector<std::vector<float>> flattenedPredictionResult;
    for (const auto &t : predictionResult)
    {
        flattenedPredictionResult.push_back(t[0].to_vector());
    }

    const auto phases = randomVectorOfDimension<float>(
        flattenedPredictionResult.size(),
        flattenedPredictionResult[0].size()
    );

    auto inputForIFFTresult = inputForIFFT<float>(flattenedPredictionResult, phases);
    REQUIRE(inputForIFFTresult.size() == predictionResult.size());
    REQUIRE(inputForIFFTresult[0].size() == predictionResult[0][0].depth());

    auto transposedForFFT = transpose<std::complex<float>>(inputForIFFTresult);
    REQUIRE(inputForIFFTresult.size() == transposedForFFT[0].size());
    REQUIRE(inputForIFFTresult[0].size() == transposedForFFT.size());

    Eigen::FFT<float> fft;
    std::vector<float> timeVector;
    for (const auto &freqVec : inputForIFFTresult)
    {
        std::vector<float> current;
        fft.inv(current, freqVec);
        timeVector.insert(timeVector.end(), current.begin(), current.end());
    }

    AudioFile<float> audioFile;
    audioFile.setAudioBufferSize(1, timeVector.size());
    audioFile.setBitDepth(24);
    audioFile.setSampleRate(22050);
    
    AudioFile<float>::AudioBuffer buffer;
    buffer.resize(1);
    buffer[0].resize(timeVector.size());
    buffer[0] = timeVector;

    audioFile.setAudioBuffer(buffer);
    audioFile.save("./audioFile.wav");
}
