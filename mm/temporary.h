#pragma once
#include <string>

namespace MM {

using Temporary = std::string;

inline bool is_label(const Temporary &temp) {
    return temp[0] == '%' && temp[1] == '.';
}

inline bool is_param(const Temporary &temp) {
    return temp[0] == '%' && temp[1] == 'p';
}

inline bool is_normal_temp(const Temporary &temp) {
    return temp[0] == '%' && !is_label(temp) && !is_param(temp);
}

inline bool is_global_function(const Temporary &temp) {
    return temp[0] == '#';
}

inline bool is_global(const Temporary &temp) {
    return temp[0] == '@';
}

inline std::string root(Temporary &temp) {
    auto pos = temp.find('.');
    return pos == std::string::npos ? temp : temp.substr(0, pos);
}

}; // namespace MM