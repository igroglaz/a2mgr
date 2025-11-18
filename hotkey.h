#pragma once

#include <cstdint>

enum class Alias : uint32_t {
    DEFAULT = 0,
    HELP = 0x0048a261, // F1 handler.
    DIPLOMACY = 0x0048a277, // F3 handler.
    // TODO: support more actions :D
};
