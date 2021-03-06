#pragma once

#include <algorithm>
#include <cassert>
#include <complex>
#include <exception>
#include <iostream>
#include <cmath>
#include <random>
#include <vector>

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
        for (std::size_t i = 0; i < lo.size(); ++i)
        {
            x_lo.push_back(mX[lo[i]]);
            x_hi.push_back(mX[hi[i]]);
        }

        std::vector<std::vector<U>> y_lo;
        std::vector<std::vector<U>> y_hi;
        for (std::size_t i = 0; i < lo.size(); ++i)
        {
            y_lo.push_back(mY[lo[i]]);
            y_hi.push_back(mY[hi[i]]);
        }

        std::vector<std::vector<U>> slopes;
        for (std::size_t i = 0; i < y_lo.size(); ++i)
        {
            std::vector<U> slopes_inner;
            for (std::size_t j = 0; j < y_lo[0].size(); ++j)
            {
                slopes_inner.push_back(
                        (y_hi[i][j] - y_lo[i][j]) / (x_hi[i] - x_lo[i])
                );
            }
            slopes.push_back(slopes_inner);
        }

        std::vector<std::vector<U>> interpolation;
        for (std::size_t i = 0; i < slopes.size(); ++i)
        {
            std::vector<U> interp_inner;
            for (std::size_t j = 0; j < slopes[i].size(); ++j)
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

template <typename T>
std::vector<T> linspace(T start, T stop, unsigned num)
{
    assert(start < stop);
    assert(num > 0);
    if (num == 2) return {start, stop};

    num -= 1;

    double delta = (stop - start) / num;
    std::vector<T> output;
    for (unsigned step = 0; step < num; ++step)
    {
        output.emplace_back(start + (step * delta));
    }
    output.emplace_back(stop);

    return output;
}

template <typename T>
std::vector<std::vector<T>> randomVectorOfDimension(unsigned firstAxis, unsigned secondAxis)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<> dist1(0,1);

    const double PI = std::acos(-1);

    std::vector<std::vector<T>> output;
    for (unsigned i = 0; i < firstAxis; ++i)
    {
        std::vector<T> current;
        for (unsigned j = 0; j < secondAxis; ++j)
        {
            current.emplace_back(dist1(rng) * 2 * PI);
        }
        output.emplace_back(std::move(current));
    }

    return output;
}

template <typename T>
std::vector<std::vector<std::complex<T>>>
inputForIFFT(std::vector<std::vector<T>> prediction, std::vector<std::vector<T>> phase)
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

    std::vector<std::vector<std::complex<T>>> output;
    for (unsigned i = 0; i < firstAxis; ++i)
    {
        std::vector<std::complex<T>> current;
        for (unsigned j = 0; j < secondAxis; ++j)
        {
            const std::complex<double> expPhase (0.0, phase[i][j]);
            const T power = ((prediction[i][j] * xMax) + sClip) / 10;
            current.emplace_back(
                std::sqrt(std::pow(10, power) * std::exp(expPhase))
            );
        }
        output.emplace_back(current);
    }
    return output;
}

template <typename T, typename U>
std::vector<std::vector<T>> inputForPrediction(int N, int T_frames)
{
    std::vector<std::vector<T>> output;
    auto z_prev = randomVectorOfDimension<U>(1, 8);
    for (int i = 0; i < N; ++i)
    {
        auto z_rand = randomVectorOfDimension<U>(1, 8);
        auto linfit = lin_interp1d_first_axis<T, U>({0, 1}, {z_prev[0], z_rand[0]});
        auto z_interp = linfit(linspace<U>(0, 1, T_frames));
        std::copy(z_interp.begin(), z_interp.end(), std::back_inserter(output));
        z_prev = z_rand;
    }
    return output;
}

template <typename T>
std::vector<std::vector<T>> transpose(const std::vector<std::vector<T>> &in)
{
    std::vector<std::vector<T>> outTransposed(
        in[0].size(),
        std::vector<T>(in.size())
    );

    for (size_t i = 0; i < in.size(); ++i)
        for (size_t j = 0; j < in[0].size(); ++j)
            outTransposed[j][i] = in[i][j];

    return outTransposed;
}
