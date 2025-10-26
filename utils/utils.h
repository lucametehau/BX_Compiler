#pragma once
#include <vector>

namespace utils {

template <typename T>
inline void concat(std::vector<T>& va, std::vector<T>& tb) {
    for (auto &x : tb)
        va.push_back(x);
}


template <typename T>
class GeneralSet {
    std::set<T> s;

public:
    GeneralSet() { s.clear(); }

    bool operator != (const GeneralSet &other) const {
        return s != other.s;
    }

    [[nodiscard]] GeneralSet join(const GeneralSet &other) const {
        GeneralSet ans = *this;
        for (auto &x : other.s)
            ans.insert(x);
        return ans;
    }

    [[nodiscard]] GeneralSet intersect(const GeneralSet &other) const {
        GeneralSet ans;
        for (auto &x : s) {
            if (other.count(x))
                ans.insert(x);
        }
        return ans;
    }

    [[nodiscard]] GeneralSet minus(const GeneralSet &other) const {
        GeneralSet ans;
        for (auto &x : s) {
            if (!other.count(x))
                ans.insert(x);
        }
        return ans;
    }

    void insert(T x) {
        s.insert(x);
    }

    [[nodiscard]] int count(const T x) const {
        return s.count(x);
    }

    std::set<T> &get_set() {
        return s;
    }
};

}; // namespace utils