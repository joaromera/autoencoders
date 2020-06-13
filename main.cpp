#include <fdeep/fdeep.hpp>

int main()
{
    const auto model = fdeep::load_model("autoencoder.h5");
    const auto result = model.predict(
        {fdeep::tensor(fdeep::tensor_shape(static_cast<float>(441)), std::vector<float>(441, 1.0f))});
    std::cout << fdeep::show_tensors(result) << std::endl;
}