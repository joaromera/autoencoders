#include <iostream>
#include <fstream>
#include <string>
#include "../AutoencoderJuce/external/json/include/nlohmann/json.hpp"

int main()
{
    std::ifstream ifs("data.json");
    nlohmann::json data;
    ifs >> data;
    // std::cout << data.at("model").dump() << std::endl;
    // float xmax = std::stof(data.at("parameters").at("xMax").get<std::string>());
    float xmax = std::stof(data["parameters"]["xMax"].get<std::string>());
    std::cout << xmax << std::endl;

    std::cout << data["parameters"]["zRange"] << std::endl;

    std::cout << data["parameters"]["zRange"].size() << std::endl;

    for (auto i : data["parameters"]["zRange"])
    {
        std::cout << i << std::endl;
    }
}