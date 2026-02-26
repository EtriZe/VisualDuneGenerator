// src/ConfigKV.h
#pragma once

#include <string>

#include "Params.h"

namespace dune {

class ConfigKV {
public:
    static bool save(const Params& P, const std::string& path);
    static bool load(Params& P, const std::string& path);

private:
    static std::string trimCopy(const std::string& s);
    static bool parseIntSafe(const std::string& s, int& out);
    static bool parseFloatSafe(const std::string& s, float& out);
    static bool parseBoolSafe(const std::string& s, bool& out);
};

} // namespace dune