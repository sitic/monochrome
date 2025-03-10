#pragma once

#include <string>
#include <variant>
#include <iostream>
#include <cstdio>
#include <stdexcept>

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

    // MacOS 10.9 and earlier do not support std::variant, so we need to use a workaround
    // Our own exception type instead of std::bad_variant_access
    class variant_access_error : public std::runtime_error {
    public:
        variant_access_error() : std::runtime_error("variant access error") {}
    };

    // Fix for older macOS versions, avoid std::get and std::visit
    inline PipeOption get_pipe_option_value(const PipeVar& var) {
        if (var.index() == static_cast<size_t>(PipeVarIndex::option)) {
            const PipeOption* pOpt = reinterpret_cast<const PipeOption*>(&var);
            return *pOpt;
        }
        return PipeOption::pipe;
    }

    inline std::string& get_string_value(PipeVar& var) {
        if (var.index() == static_cast<size_t>(PipeVarIndex::string)) {
            return *reinterpret_cast<std::string*>(&var);
        }
        throw variant_access_error();
    }

    inline PipeHandle get_handle_value(PipeVar& var) {
        if (var.index() == static_cast<size_t>(PipeVarIndex::handle)) {
            return *reinterpret_cast<PipeHandle*>(&var);
        }
        throw variant_access_error();
    }

    inline std::istream* get_istream_value(PipeVar& var) {
        if (var.index() == static_cast<size_t>(PipeVarIndex::istream)) {
            return *reinterpret_cast<std::istream**>(&var);
        }
        throw variant_access_error();
    }

    inline std::ostream* get_ostream_value(PipeVar& var) {
        if (var.index() == static_cast<size_t>(PipeVarIndex::ostream)) {
            return *reinterpret_cast<std::ostream**>(&var);
        }
        throw variant_access_error();
    }

    inline FILE* get_file_value(PipeVar& var) {
        if (var.index() == static_cast<size_t>(PipeVarIndex::file)) {
            return *reinterpret_cast<FILE**>(&var);
        }
        throw variant_access_error();
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