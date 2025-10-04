#pragma once
#include <vector>

namespace utils {

template <typename T>
inline void concat(std::vector<T>& va, std::vector<T>& tb) {
    for (auto &x : tb)
        va.push_back(x);
}

}; // namespace utils