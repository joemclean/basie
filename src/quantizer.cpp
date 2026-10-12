#include "quantizer.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Quantizer {
Note quantize(float inputVoltage, Theory::NoteMask notes) {
    if (!std::isfinite(inputVoltage)) inputVoltage = 0;
    inputVoltage = std::clamp(inputVoltage, 0.f, maxVoltage);
    const float inputSemitones = inputVoltage * 12.f;
    const int octave = static_cast<int>(std::floor(inputVoltage));
    int best = 0;
    float distance = std::numeric_limits<float>::infinity();
    // Search absolute pitches, including neighboring octaves.
    for (int candidate = std::max(0, (octave - 1) * 12);
         candidate <= std::min(60, (octave + 2) * 12 - 1); ++candidate) {
        if (!Theory::contains(notes, candidate % 12)) continue;
        const float delta = std::abs(inputSemitones - static_cast<float>(candidate));
        if (delta < distance) { distance = delta; best = candidate; }
    }
    // An empty mask is the explicit no-song state, producing 0 V.
    return {static_cast<float>(best) / 12.f, best % 12, best / 12};
}
std::uint16_t dacCode(float voltage) {
    if (!std::isfinite(voltage)) return 0;
    // Preserve nominal codes/V, saturating the 5 V endpoint.
    const float code = std::round(std::clamp(voltage, 0.f, maxVoltage) * 819.2f);
    return static_cast<std::uint16_t>(std::min(code, 4095.f));
}
} // namespace Quantizer
