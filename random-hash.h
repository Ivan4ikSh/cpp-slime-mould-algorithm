#pragma once
#include "domain.h"

uint32_t Hash(uint32_t state) {
    state ^= 2747636219u;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    state ^= state >> 16;
    return state * 2654435769u;
}

float ScaleToRange01(uint32_t value) {
    return static_cast<float>(value) / 4294967295.0f;
}