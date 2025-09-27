#pragma once
#include <map>
#include <string>
#include <cassert>
#include <vector>
#pragma message(">>> NEW mm.h is being compiled <<<")


namespace MM {

class Scope {
private:
    std::map<std::string, std::string> temp_map;

public:
    void declare(std::string name, std::string temp) {
        temp_map[name] = temp;
    }

    [[nodiscard]] std::string get_temp(const std::string& name) const {
        auto it = temp_map.find(name);
        assert (it != temp_map.end());
        return it->second;
    }

    [[nodiscard]] bool is_declared(const std::string& name) const {
        auto it = temp_map.find(name);
        return it != temp_map.end();
    }
};

class MM {
    int temp_ind;
    std::vector<Scope> scopes;

public:
    MM() : temp_ind(0) {}

    void push_scope() { scopes.emplace_back(); }

    void pop_scope() { scopes.pop_back(); }

    Scope& scope() { 
        assert(!scopes.empty());
        return scopes.back();
    }

    [[nodiscard]] std::string new_temp()  {
        return "%" + std::to_string(temp_ind++);
    }
};

}; // namespace MM