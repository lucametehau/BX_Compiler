#pragma once
#include "../lexer/lexer.h"
#include <map>
#include <utility>
#include <vector>

namespace MM {

using Temporary = std::string;

struct Type {
    enum Kind {
        INT,
        BOOL,
        VOID,
        FUNCTION
    } kind;

    std::vector<Type> param_types;
    std::unique_ptr<Type> return_type; // todo, make unique_ptr

    Type() = default;
    Type(Kind k) : kind(k) {}

    Type(const Type& other) : kind(other.kind), param_types(other.param_types) {
        if (other.return_type)
            return_type = std::make_unique<Type>(*other.return_type);
    }

    Type& operator=(const Type& other) {
        if (this != &other) {
            kind = other.kind;
            param_types = other.param_types;
            if (other.return_type)
                return_type = std::make_unique<Type>(*other.return_type);
            else
                return_type = nullptr;
        }
        return *this;
    }

    static Type Int()  { return Type(INT); }
    static Type Bool() { return Type(BOOL); }
    static Type Void() { return Type(VOID); }

    static Type Function(std::vector<Type> params, Type ret) {
        Type t(FUNCTION);
        t.param_types = std::move(params);
        t.return_type = std::make_unique<Type>(ret);
        return t;
    }

    bool operator == (const Type &other) const {
        if (kind != other.kind)
            return false;
        
        return kind != FUNCTION || 
               (param_types == other.param_types && 
               *return_type == *other.return_type);
    }

    constexpr bool is_int() const { return kind == INT; }
    constexpr bool is_bool() const { return kind == BOOL; }
    constexpr bool is_void() const { return kind == VOID; }

    constexpr bool is_first_order() const {
        return kind == INT || kind == BOOL || kind == VOID;
    }

    constexpr bool is_function() const { return kind == FUNCTION; }

    const Type get_param_type(std::size_t ind) const {
        assert (kind == FUNCTION);
        assert (ind < param_types.size());
        return param_types[ind];
    }

    const Type get_return_type() const {
        assert (kind == FUNCTION);
        assert (return_type);
        return *return_type;
    }

    std::size_t get_num_params() const {
        return param_types.size();
    }

    std::string to_string() const {
        switch (kind) {
            case INT: return "int";
            case BOOL: return "bool";
            case VOID: return "void";
            case FUNCTION: {
                std::string s = "function(";
                for (size_t i = 0; i < param_types.size(); i++) {
                    s += param_types[i].to_string();
                    if (i + 1 < param_types.size()) s += ", ";
                }
                s += ") -> " + return_type->to_string();
                return s;
            }
        }
        return "<?>";
    }
};

inline std::map<lexer::Type, Type> lexer_to_mm_type = {
    {lexer::INT, Type::Int()}, {lexer::BOOL, Type::Bool()}, {lexer::VOID, Type::Void()}
};

}; // namespace MM