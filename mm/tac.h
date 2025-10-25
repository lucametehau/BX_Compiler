#pragma once
#include <map>
#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <optional>


class TAC {
private:
    std::string opcode;
    std::vector<std::string> args;
    std::optional<std::string> result;

public:
    TAC() = default;
    TAC(std::string opcode, std::vector<std::string> args) : opcode(std::move(opcode)), args(std::move(args)) {}
    TAC(std::string opcode, std::vector<std::string> args, std::string result) : opcode(std::move(opcode)), args(std::move(args)), result(std::move(result)) {}

    friend std::ostream& operator << (std::ostream& os, TAC& tac) {
        os << "{\"opcode\": \"" << tac.opcode << "\", \"args\": [";
        for (std::size_t i = 0; i + 1 < tac.args.size(); i++) {
            os << "\"" << tac.args[i] << "\", ";
        }
        if (!tac.args.empty() && !(tac.args.back()[0] >= '0' && tac.args.back()[0] <= '9'))
            os << "\"" << tac.args.back() << "\"";
        else if (!tac.args.empty())
            os << std::stoi(tac.args.back());
        os << "], \"result\": ";
        if (!tac.has_result())
            os << "null";
        else
            os << "\"" << tac.get_result() << "\"";
        os << "}";
        return os;
    }

    [[nodiscard]] bool has_result() const {
        return result.has_value();
    }

    [[nodiscard]] std::string get_opcode() const {
        return opcode;
    }

    [[nodiscard]] std::vector<std::string> get_args() const {
        return args;
    }

    [[nodiscard]] std::string get_arg() const {
        assert(!args.empty());
        return args[0];
    }

    [[nodiscard]] std::string get_result() const {
        assert(result.has_value());
        return result.value();
    }

    void set_result(std::string _result) {
        result = _result;
    }

    void set_arg(std::string arg) {
        args = { arg };
    }
};