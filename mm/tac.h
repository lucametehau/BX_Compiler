#pragma once
#include <map>
#include <vector>
#include <string>
#include <cassert>


class TAC {
private:
    std::map<std::string, std::vector<std::string>> instruction;

public:
    TAC() = default;
    TAC(std::map<std::string, std::vector<std::string>>& instruction) : instruction(std::move(instruction)) {}

    std::vector<std::string>& operator [] (const std::string& s) {
        return instruction[s];
    }

    friend std::ostream& operator << (std::ostream& os, TAC& tac) {
        os << "{\"opcode\": \"" << tac["opcode"][0] << "\", \"args\": [";
        for (std::size_t i = 0; i + 1 < tac["args"].size(); i++)
            os << "\"" << tac["args"][i] << "\", ";
        if (!tac["args"].empty() && tac["args"].back()[0] == '%')
            os << "\"" << tac["args"].back() << "\"";
        else
            os << std::stoi(tac["args"].back());
        os << "], \"result\": ";
        if (tac["result"].empty())
            os << "null";
        else
            os << "\"" << tac["result"][0] << "\"";
        os << "}";
        return os;
    } 
};