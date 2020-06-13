#include <fdeep/fdeep.hpp>

constexpr size_t DEPTH = 441;

int main()
{
    // un input cualquiera para probar tiene DEPTH floats de valor 1.0f
    std::vector<float> input (DEPTH, 1.0f);

    fdeep::tensor_shape depth (DEPTH);
    fdeep::tensors tensors = {fdeep::tensor(depth, input)};

    const auto model = fdeep::load_model("autoencoder.h5");
    const auto result = model.predict(tensors);

    std::cout << fdeep::show_tensors(result) << std::endl;
}