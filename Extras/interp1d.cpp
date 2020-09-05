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
                std::cout << e << " at index " << std::distance(mX.begin(), lower) << std::endl;
                new_indexes.emplace_back(std::distance(mX.begin(), lower));
            }
            else
            {
                std::cout << e << " at index " << mX.size() << std::endl;
                new_indexes.emplace_back(mX.size());
            }
        }

        // Clip x_new_indices so that they are within the range of 
        // mX indices and at least 1. Removes mis-interpolation
        // xnew[n] = mX[0]
        for_each(new_indexes.begin(), new_indexes.end(), [&] (auto &e) {
            int hi = mX.size() - 1;
            e = std::clamp(e, 1, hi);
        });

        // Calculate the slope of regions that each x_new value falls in.
        std::vector<int> lo = new_indexes;
        for_each(lo.begin(), lo.end(), [] (auto &e) {
            --e;
        });
        const std::vector<int> &hi = new_indexes;

        std::vector<T> x_lo;
        std::vector<T> x_hi;
        for (int i = 0; i < mX.size(); ++i)
        {
            x_lo.push_back(mX[lo[i]]);
            x_hi.push_back(mX[hi[i]]);
        }

        std::vector<std::vector<U>> y_lo;
        std::vector<std::vector<U>> y_hi;
        for (int i = 0; i < mY.size(); ++i)
        {
            y_lo.push_back(mY[lo[i]]);
            y_hi.push_back(mY[hi[i]]);
        }

        std::vector<std::vector<U>> slopes;
        for (int i = 0; i < mY.size(); ++i)
        {
            std::vector<U> slopes_inner;
            for (int j = 0; j < y_lo.size(); ++j)
            {
                slopes_inner.push_back(
                    (y_hi[i][j] - y_lo[i][j]) / (x_hi[j] - x_lo[j])
                );
            }
            slopes.push_back(slopes_inner);
        }

        std::vector<std::vector<U>> interpolation;
        for (int i = 0; i < mY.size(); ++i)
        {
            std::vector<U> interp_inner;
            for (int j = 0; j < y_lo.size(); ++j)
            {
                interp_inner.push_back(
                    (slopes[i][j] * (xnew[j] - x_lo[j])) + y_lo[i][j]
                );
            }
            interpolation.push_back(interp_inner);
        }

        for (const auto &e : interpolation)
        {
            for (const auto &f : e)
            {
                std::cout << f << std::endl;
            }
        }

        // Return something
        return interpolation;
    }

    std::vector<T> mX;
    std::vector<std::vector<U>> mY;
};

int main()
{
    std::vector<int> x = {0,1};
    std::vector<std::vector<float>> y = {
        {1.f,2.f,3.f,4.f},
        {5.f,6.f,7.f,8.f}
    };

    // // Tests with x = {0}
    auto test = lin_interp1d_first_axis<int, float>({0,1}, y);
    // assert((test.mY.size() == 1));
    // assert((test.mY[0] == std::vector<float>{0,1,2}));

    // // Tests with x = {1}
    // test = lin_interp1d_first_axis<int, float>({1}, y);
    // assert((test.mY.size() == 1));
    // assert((test.mY[0] == std::vector<float>{3,4,5}));

    // // Tests with x = {0,1}
    // test = lin_interp1d_first_axis<int, float>(x, y);
    // assert((test.mY.size() == 2));
    // assert((test.mY[0] == std::vector<float>{0,1,2}));
    // assert((test.mY[1] == std::vector<float>{3,4,5}));

    test({0.25f,0.5f,0.75f,1.f});
}