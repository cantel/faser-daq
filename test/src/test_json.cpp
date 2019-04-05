#include "Utilities/json.hpp"

using nlohmann::json;

struct process {
    std::string name = "";
};

struct config {
    process p[2];
};

int main(int argc, char const *argv[]) {
    json j = {{{"name", "ciao1"}}, {{"name", "ciao2"}}};
    std::cout << j.dump() << std::endl;

    for (int i = 0; i < 3; i++) {
        config c{j["proc1:name"].get<std::string>(),
                 j["proc2:name"].get<std::string>()};
        std::cout << c.p[0].name << std::endl;
        std::cout << c.p[1].name << std::endl;
    }

    return 0;
}
