#pragma once

#include <algorithm>
#include <cassert>
#include <complex>
#include <exception>
#include <iostream>
#include <cmath>
#include <random>
#include <vector>

using complexDouble = std::complex<double>;

template<typename T, typename U>
struct lin_interp1d_first_axis
{
    lin_interp1d_first_axis(const std::vector<T> &x, const std::vector<std::vector<U>> &y)
            : mX(x)
    {
        std::sort(mX.begin(), mX.end());

        for (const auto &idx : mX)
        {
            mY.emplace_back(y[idx]);
        }
    }

    std::vector<std::vector<U>> operator()(std::vector<U> xnew)
    {
        // Find where in the original data, the values to interpolate would be inserted.
        std::vector<int> new_indexes;
        for (const auto &e : xnew)
        {
            auto lower = std::lower_bound(mX.begin(), mX.end(), e);
            if (lower != mX.end())
            {
//                std::cout << e
//                          << " at index "
//                          << std::distance(mX.begin(), lower)
//                          << std::endl;
                new_indexes.emplace_back(std::distance(mX.begin(), lower));
            }
            else
            {
//                std::cout << e
//                          << " at index "
//                          << mX.size()
//                          << std::endl;
                new_indexes.emplace_back(mX.size());
            }
        }

        // Clip x_new_indices so that they are within the range of
        // mX indices and at least 1. Removes mis-interpolation
        // xnew[n] = mX[0]
        for_each(new_indexes.begin(), new_indexes.end(), [&](auto &e) {
            int hi = mX.size() - 1;
            e = std::clamp(e, 1, hi);
        });

        // Calculate the slope of regions that each x_new value falls in.
        std::vector<int> lo = new_indexes;
        for_each(lo.begin(), lo.end(), [](auto &e) {
            --e;
        });
        const std::vector<int> &hi = new_indexes;

        std::vector<T> x_lo;
        std::vector<T> x_hi;
        for (int i = 0; i < lo.size(); ++i)
        {
            x_lo.push_back(mX[lo[i]]);
            x_hi.push_back(mX[hi[i]]);
        }

        std::vector<std::vector<U>> y_lo;
        std::vector<std::vector<U>> y_hi;
        for (int i = 0; i < lo.size(); ++i)
        {
            y_lo.push_back(mY[lo[i]]);
            y_hi.push_back(mY[hi[i]]);
        }

        std::vector<std::vector<U>> slopes;
        for (int i = 0; i < y_lo.size(); ++i)
        {
            std::vector<U> slopes_inner;
            for (int j = 0; j < y_lo[0].size(); ++j)
            {
                slopes_inner.push_back(
                        (y_hi[i][j] - y_lo[i][j]) / (x_hi[i] - x_lo[i])
                );
            }
            slopes.push_back(slopes_inner);
        }

        std::vector<std::vector<U>> interpolation;
        for (int i = 0; i < slopes.size(); ++i)
        {
            std::vector<U> interp_inner;
            for (int j = 0; j < slopes[i].size(); ++j)
            {
                interp_inner.push_back(
                        (slopes[i][j] * (xnew[i] - x_lo[i])) + y_lo[i][j]
                );
            }
            interpolation.push_back(interp_inner);
        }

        return interpolation;
    }

    std::vector<T> mX;
    std::vector<std::vector<U>> mY;
};

std::vector<double> linspace(double start, double stop, unsigned num)
{
    assert(start < stop);
    assert(num > 0);
    if (num == 2) return {start, stop};

    num -= 1;

    double delta = (stop - start) / num;
    std::vector<double> output;
    for (unsigned step = 0; step < num; ++step)
    {
        output.emplace_back(start + (step * delta));
    }
    output.emplace_back(stop);

    return output;
}

std::vector<std::vector<double>> randomVectorOfDimension(unsigned firstAxis, unsigned secondAxis)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<> dist1(0,1);

    const double PI = std::acos(-1);

    std::vector<std::vector<double>> output;
    for (unsigned i = 0; i < firstAxis; ++i)
    {
        std::vector<double> current;
        for (unsigned j = 0; j < secondAxis; ++j)
        {
            current.emplace_back(dist1(rng) * 2 * PI);
        }
        output.emplace_back(std::move(current));
    }

    return output;
}

std::vector<std::vector<complexDouble>>
inputForIFFT(std::vector<std::vector<double>> prediction, std::vector<std::vector<double>> phase)
{
    if (prediction.size() != phase.size())
        throw std::invalid_argument("Wrong dimesntion");

    if (prediction[0].size() != phase[0].size())
        throw std::invalid_argument("Wrong dimesntion");

    using namespace std::complex_literals;

    // TODO: check these values
    const double xMax = 96;
    const double sClip = -60;

    const unsigned firstAxis = prediction.size();
    const unsigned secondAxis = prediction[0].size();

    std::vector<std::vector<complexDouble>> output;
    for (unsigned i = 0; i < firstAxis; ++i)
    {
        std::vector<complexDouble> current;
        for (unsigned j = 0; j < secondAxis; ++j)
        {
            const complexDouble expPhase = 1i * phase[i][j];
            const double power = ((prediction[i][j] * xMax) + sClip) / 10;
            current.emplace_back(
                std::sqrt(std::pow(10, power) * std::exp(expPhase))
            );
        }
        output.emplace_back(std::move(current));
    }
    return output;
}
