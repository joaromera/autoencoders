#define CATCH_CONFIG_MAIN
#include "catch.hh"

#include "math.h"
#include "interp1d.hpp"

class Interp1dTestsFixture
{
public:
    std::vector<int> zeroToOne = {0, 1};
    std::vector<float> oneToFour = {1.f, 2.f, 3.f, 4.f};
    std::vector<float> fiveToEight = {5.f, 6.f, 7.f, 8.f};

    std::vector<std::vector<float>> y = {
        oneToFour,
        fiveToEight
    };

    lin_interp1d_first_axis<int, float> zeroToOneAndY {zeroToOne, y};

    std::vector<float> oneToEight = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};
    std::vector<std::vector<float>> y2 = {
        oneToEight,
        oneToEight
    };

    lin_interp1d_first_axis<int, float> zeroToOneAndY2 {zeroToOne, y2};
};

TEST_CASE_METHOD(Interp1dTestsFixture, "Interp1dTestsWithXBeingZero", "[construction]")
{
    auto test = lin_interp1d_first_axis<int, float>({0}, y);
    REQUIRE(test.mY.size() == 1);
    REQUIRE(test.mY[0] == oneToFour);
}

TEST_CASE_METHOD(Interp1dTestsFixture, "Interp1dTestsWithXBeingOne", "[construction]")
{
    auto test = lin_interp1d_first_axis<int, float>({1}, y);
    REQUIRE(test.mY.size() == 1);
    REQUIRE(test.mY[0] == fiveToEight);
}

TEST_CASE_METHOD(Interp1dTestsFixture, "Interp1dTestsWithXBeingZeroAndOne", "[construction]")
{
    REQUIRE(zeroToOneAndY.mY.size() == 2);
    REQUIRE(zeroToOneAndY.mY[0] == oneToFour);
    REQUIRE(zeroToOneAndY.mY[1] == fiveToEight);
}

TEST_CASE_METHOD(Interp1dTestsFixture, "InterpolationWithFour", "[calling]")
{
    auto interpolation = zeroToOneAndY({0.25f,0.5f,0.75f,1.f});
    REQUIRE(interpolation.size() == 4);
    REQUIRE(interpolation[0].size() == 4);
    REQUIRE(interpolation[0] == std::vector<float>{2.f, 3.f, 4.f, 5.f});
    REQUIRE(interpolation[1] == std::vector<float>{3.f, 4.f, 5.f, 6.f});
    REQUIRE(interpolation[2] == std::vector<float>{4.f, 5.f, 6.f, 7.f});
    REQUIRE(interpolation[3] == std::vector<float>{5.f, 6.f, 7.f, 8.f});
}

TEST_CASE_METHOD(Interp1dTestsFixture, "InterpolationWithTwo", "[calling]")
{
    auto interpolation = zeroToOneAndY({0.25f, 0.5f});
    REQUIRE(interpolation.size() == 2);
    REQUIRE(interpolation[0].size() == 4);
    REQUIRE(interpolation[0] == std::vector<float>{2.f, 3.f, 4.f, 5.f});
    REQUIRE(interpolation[1] == std::vector<float>{3.f, 4.f, 5.f, 6.f});
}

TEST_CASE_METHOD(Interp1dTestsFixture, "InterpolationWithTwoAndBiggerYZ", "[construction]")
{
    REQUIRE(zeroToOneAndY2.mY.size() == 2);
    REQUIRE(zeroToOneAndY2.mY[0] == oneToEight);
    REQUIRE(zeroToOneAndY2.mY[1] == oneToEight);
}

TEST_CASE_METHOD(Interp1dTestsFixture, "InterpolationWithTwoAndBiggerY", "[calling]")
{
    auto interpolation = zeroToOneAndY2({0.1f, 0.2f, 0.3f, 0.4f, 0.5f});
    REQUIRE(interpolation.size() == 5);
    REQUIRE(interpolation[0].size() == 8);
    REQUIRE(interpolation[0] == oneToEight);
    REQUIRE(interpolation[0] == interpolation[1]);
    REQUIRE(interpolation[1] == interpolation[2]);
    REQUIRE(interpolation[2] == interpolation[3]);
    REQUIRE(interpolation[3] == interpolation[4]);
}

TEST_CASE("Linspace has correct size when asked for two numbers","[Basic Tests")
{
    auto linspaceResult = linspace(0, 1, 2);
    REQUIRE(linspaceResult.size() == 2);
}


TEST_CASE("Linspace has correct output with arguemnts 0, 1, 2","[Basic Tests")
{
    auto linspaceResult = linspace(0, 1, 2);
    REQUIRE(linspaceResult[0] == 0);
    REQUIRE(linspaceResult[1] == 1);
}

TEST_CASE("Linspace has correct size ","[Basic Tests")
{
    auto linspaceResult = linspace(0, 1, 10);
    REQUIRE(linspaceResult.size() == 10);
}

TEST_CASE("Linspace output has start and stop ","[Basic Tests")
{
    auto linspaceResult = linspace(0, 1, 10);
    REQUIRE(linspaceResult[0] == 0);
    REQUIRE(linspaceResult[9] == 1);
}

TEST_CASE("Linspace has correct output between 0 and 5 ","[Basic Tests")
{
    auto linspaceResult = linspace(0, 1, 5);
    REQUIRE(linspaceResult[0] == 0);
    REQUIRE(linspaceResult[1] == 0.25);
    REQUIRE(linspaceResult[2] == 0.50);
    REQUIRE(linspaceResult[3] == 0.75);
    REQUIRE(linspaceResult[4] == 1);
}

TEST_CASE("randomVectorOfDimension", "[Basic Tests]")
{
    const double maxValue = 2 * M_PI;
    auto randomVec = randomVectorOfDimension(1,2);
    REQUIRE(randomVec.size() == 1);
    REQUIRE(randomVec[0].size() == 2);
    REQUIRE(randomVec[0][0] < maxValue);
    REQUIRE(randomVec[0][1] < maxValue);
}

TEST_CASE("Input for IFFT", "[Basic Tests")
{
    std::vector<double> fa = {-0.1, 0.2, -0.3, 0.4};
    std::vector<double> sa = {0.5,- 0.6, 0.7, -0.8};

    std::vector<std::vector<double>> i = {
        fa,
        sa
    };

    auto input = inputForIFFT(i, i);

//    for (const auto &vector_c : input)
//    {
//        for (const auto &c : vector_c)
//        {
//            std::cout << c << std::endl;
//        }
//    }
}

TEST_CASE("Input for prediction", "[Calling]")
{
    auto test = inputForPrediction<double, double>(12, 5);
    REQUIRE(test.size() == 60);

    const double maxValue = 2 * M_PI;
    for (const auto &v : test)
    {
        REQUIRE(v.size() == 8);
        for (const auto &e : v)
        {
            REQUIRE(e < maxValue);
        }
    }
}
