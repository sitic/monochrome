#pragma once

#include <string>
#include <variant>
#include <iostream>
#include <cstdio>

#include "basic_types.hpp"


namespace subprocess {
    enum class PipeVarIndex {
        option,
        string,
        handle,
        istream,
        ostream,
        file
    };

    typedef std::variant<PipeOption, std::string, PipeHandle,
        std::istream*, std::ostream*, FILE*> PipeVar;

    // Fix for older macOS versions, don't use std::get as upstream does
    inline PipeOption get_pipe_option_value(const PipeVar& var) {
        if (var.index() == static_cast<size_t>(PipeVarIndex::option)) {
            return std::visit([](auto&& arg) -> PipeOption {
                if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, PipeOption>)
                    return arg;
                return PipeOption::pipe;
            }, var);
        }
        return PipeOption::pipe;
    }

    inline std::string& get_string_value(PipeVar& var) {
        return std::visit([](auto&& arg) -> std::string& {
            if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, std::string>)
                return arg;
            throw std::bad_variant_access();
        }, var);
    }

    inline PipeHandle get_handle_value(PipeVar& var) {
        return std::visit([](auto&& arg) -> PipeHandle {
            if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, PipeHandle>)
                return arg;
            throw std::bad_variant_access();
        }, var);
    }

    inline std::istream* get_istream_value(PipeVar& var) {
        return std::visit([](auto&& arg) -> std::istream* {
            if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, std::istream*>)
                return arg;
            throw std::bad_variant_access();
        }, var);
    }

    inline std::ostream* get_ostream_value(PipeVar& var) {
        return std::visit([](auto&& arg) -> std::ostream* {
            if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, std::ostream*>)
                return arg;
            throw std::bad_variant_access();
        }, var);
    }

    inline FILE* get_file_value(PipeVar& var) {
        return std::visit([](auto&& arg) -> FILE* {
            if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, FILE*>)
                return arg;
            throw std::bad_variant_access();
        }, var);
    }

    inline PipeOption get_pipe_option(const PipeVar& option) {
        PipeVarIndex index = static_cast<PipeVarIndex>(option.index());

        switch(index) {
        case PipeVarIndex::option:  return get_pipe_option_value(option);
        case PipeVarIndex::handle:  return PipeOption::specific;

        default:                    return PipeOption::pipe;
        }
    }
}