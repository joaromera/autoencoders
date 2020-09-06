#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
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
