#include <iostream>
#include <fstream>
#include <string>
#include "../AutoencoderJuce/external/json/include/nlohmann/json.hpp"

int main()
{
    std::ifstream ifs("data.json");
    nlohmann::json data;
    ifs >> data;
    std::cout << data.at("model").dump() << std::endl;
    float xmax = std::stof(data.at("xMax").get<std::string>());
    std::cout << xmax << std::endl;
}