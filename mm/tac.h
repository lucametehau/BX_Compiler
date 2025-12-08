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
    std::map<std::string, std::string> phi_args;

public:
    TAC() = default;
    TAC(std::string opcode, std::vector<std::string> args) : opcode(std::move(opcode)), args(std::move(args)) {}
    TAC(std::string opcode, std::vector<std::string> args, std::string result) : opcode(std::move(opcode)), args(std::move(args)), result(std::move(result)) {}
    TAC(std::string opcode, std::map<std::string, std::string> &phi_args, std::string result = "") : opcode(std::move(opcode)), result(std::move(result)), phi_args(std::move(phi_args)) {}

    friend std::ostream& operator << (std::ostream& os, TAC& tac) {
        if (tac.opcode == "phi") {
            os << "phi(";
            for (auto &[label, temp] : tac.phi_args) {
                os << label << ": " << temp << ", ";
            }
            os << ") -> ";
            os << tac.result.value() << "\n";
            return os;
        }
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

    [[nodiscard]] std::vector<std::string> &get_args() {
        return args;
    }

    [[nodiscard]] std::map<std::string, std::string> get_phi_args() const {
        return phi_args;
    }

    [[nodiscard]] std::map<std::string, std::string> &get_phi_args() {
        return phi_args;
    }

    [[nodiscard]] std::string get_arg() const {
        assert(!args.empty());
        return args[0];
    }

    [[nodiscard]] std::string &get_arg() {
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