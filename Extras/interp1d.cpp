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
                    (slopes[i][j] * (xnew[i] - x_lo[j])) + y_lo[i][j]
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

    double delta = (stop - start) / num;
    std::vector<double> output;
    for (double step = start; step < stop; step += delta)
    {
        output.emplace_back(step);
    }
    output.emplace_back(stop);

    return output;
}

int main()
{
    std::vector<int> x = {0, 1};
    std::vector<std::vector<float>> y = {
        {1.f, 2.f, 3.f, 4.f},
        {5.f, 6.f, 7.f, 8.f}
    };

    // Tests with x = {0}
    auto test = lin_interp1d_first_axis<int, float>({0}, y);
    assert((test.mY.size() == 1));
    assert((test.mY[0] == std::vector<float>{1.f, 2.f, 3.f, 4.f}));

    // Tests with x = {1}
    test = lin_interp1d_first_axis<int, float>({1}, y);
    assert((test.mY.size() == 1));
    assert((test.mY[0] == std::vector<float>{5.f, 6.f, 7.f, 8.f}));

    // Tests with x = {0,1}
    test = lin_interp1d_first_axis<int, float>(x, y);
    assert((test.mY.size() == 2));
    assert((test.mY[0] == std::vector<float>{1.f, 2.f, 3.f, 4.f}));
    assert((test.mY[1] == std::vector<float>{5.f, 6.f, 7.f, 8.f}));

    auto interpolation = test({0.25f,0.5f,0.75f,1.f});
    assert((interpolation.size() == 4));
    assert((interpolation[0].size() == 4));
    assert((interpolation[0] == std::vector<float>{2.f, 3.f, 4.f, 5.f}));
    assert((interpolation[1] == std::vector<float>{3.f, 4.f, 5.f, 6.f}));
    assert((interpolation[2] == std::vector<float>{4.f, 5.f, 6.f, 7.f}));
    assert((interpolation[3] == std::vector<float>{5.f, 6.f, 7.f, 8.f}));

    interpolation = test({0.25f, 0.5f});
    assert((interpolation.size() == 2));
    assert((interpolation[0].size() == 4));
    assert((interpolation[0] == std::vector<float>{2.f, 3.f, 4.f, 5.f}));
    assert((interpolation[1] == std::vector<float>{3.f, 4.f, 5.f, 6.f}));

    y = {
        {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f},
        {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f}
    };

    test = lin_interp1d_first_axis<int, float>(x, y);
    assert((test.mY.size() == 2));
    assert((test.mY[0] == std::vector<float>{1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f}));
    assert((test.mY[1] == std::vector<float>{1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f}));

    interpolation = test({0.1f, 0.2f, 0.3f, 0.4f, 0.5f});
    assert((interpolation.size() == 5));
    assert((interpolation[0].size() == 8));
    assert((interpolation[0] == std::vector<float>{1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f}));
    assert((interpolation[0] == interpolation[1]));
    assert((interpolation[1] == interpolation[2]));
    assert((interpolation[2] == interpolation[3]));
    assert((interpolation[3] == interpolation[4]));

    auto linspaceFrom0To1 = linspace(0, 1, 10);

}
