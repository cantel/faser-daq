#include <iostream>
#include "nlohmann/json.hpp"

using nlohmann::json;

struct process {
  std::string name;
  std::string type;
};

std::vector<process> processes;

// void to_json(json& j, const std::vector<process>& c) { 
//   j = json{{"name", p.name}, {"address", p.address}, {"age", p.age}};
// }

void from_json(const json& j, std::vector<process>& ps) {
  std::cout << "range-based for" << std::endl;
  for (auto& element : j) {
    process p;
    std::cout << element << '\n';
    element.at("name").get_to(p.name);
    element.at("type").get_to(p.type);
    ps.push_back(p);
  }
}

int main(int argc, char const* argv[]) {
  json j = {{{"name", "Luke"}, {"type", "Jedi"}},
            {{"name", "Han"}, {"type", "Mercenary"}},
            {{"name", "Leia"}, {"type", "Princess"}}};
 
  std::cout << "size " << j.size() << std::endl;
  std::cout << "is_array " << j.is_array() << std::endl;
  std::cout << j.dump() << std::endl;

  std::vector<process> ps;
  from_json(j, ps);
  std::cout << "Let's check what's in the struct!" << std::endl;
  for (auto& element : ps) {
      std::cout << element.name << std::endl;
      std::cout << element.type << std::endl;
  }

  return 0;
}
