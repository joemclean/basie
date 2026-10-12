#pragma once
#include "theory.hpp"
#include <cstdint>

namespace Quantizer {
constexpr float maxVoltage = 5.f;
struct Note {
    float voltage = 0;
    int pitchClass = 0;
    int octave = 0;
};
// Nearest enabled note within 0..5 V. Exact ties select the lower note.
Note quantize(float inputVoltage, Theory::NoteMask notes);
std::uint16_t dacCode(float voltage);
} // namespace Quantizer
