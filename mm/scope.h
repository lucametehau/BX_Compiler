#pragma once
#include <map>
#include <string>
#include <optional>
#include "type.h"

namespace MM {

struct Symbol {
    std::string name;
    Type type;
    Temporary temp; // temporary
};

class Scope {
private:
    std::map<std::string, Symbol> temp_map;
    std::optional<std::pair<std::string, Type>> function;

public:
    void declare(std::string name, Type type, Temporary temp) {
#ifdef DEBUG
        std::cout << "Declared " << name << " with type " << type.to_string() << "\n";
#endif
        temp_map[name] = Symbol{name, type, temp};
    }

    // sets the type of the function in which the scope is
    void set_function(std::string name, Type type) {
        function = {name, type};
    }

    [[nodiscard]] std::string get_temp(const std::string& name) const {
        auto it = temp_map.find(name);
        assert (it != temp_map.end());
        return it->second.temp;
    }

    [[nodiscard]] Type get_type(const std::string& name) const {
        auto it = temp_map.find(name);
        assert (it != temp_map.end());
        return it->second.type;
    }

    [[nodiscard]] std::optional<std::string> get_current_function_name() const {
        if (!function.has_value()) return std::nullopt;
        return std::optional<std::string>(function.value().first);
    }

    [[nodiscard]] std::optional<Type> get_current_function_type() const {
        if (!function.has_value()) return std::nullopt;
        return std::optional<Type>(function.value().second);
    }

    [[nodiscard]] bool is_declared(const std::string& name) const {
        auto it = temp_map.find(name);
        return it != temp_map.end();
    }
};

}; // namespace MM