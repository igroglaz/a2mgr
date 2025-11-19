#pragma once

#include <cstdint>

enum class Action : uint32_t {
    DEFAULT = 0,

    // Top-level actions. All in the function at 0x0048a097.
    HELP = 0x0048a261, // F1 handler.
    DIPLOMACY = 0x0048a277, // F3 handler.

    // Unit actions. All within a single switch at 0x0040ccc8.
    UNIT_A = 0x0040cce3,
    UNIT_C = 0x0040cd47,
    UNIT_D = 0x0040cd67,
    UNIT_G = 0x0040ccf7,
    UNIT_L = 0x0040cd8d,
    UNIT_M = 0x0040cccf,
    UNIT_P = 0x0040cd7b,
    UNIT_R = 0x0040cd1f,
    UNIT_S = 0x0040cd33,
    UNIT_T = 0x0040cd0b,

    // Other actions.
    ACTION_B = 0x0040ce12,
    ACTION_C = 0x0040d2a5,
    ACTION_E = 0x0040d27e,
    ACTION_F = 0x0040d016,
    ACTION_H = 0x0040d05e,
    ACTION_I = 0x0040cdcd,
    ACTION_K = 0x0040d222,
    ACTION_L = 0x0040d12e,
    ACTION_N = 0x0040d178,
    ACTION_O = 0x0040d1d2,
    ACTION_T = 0x0040d288,
    ACTION_U = 0x0040d0a8,
    ACTION_W = 0x0040cfaf,
};
