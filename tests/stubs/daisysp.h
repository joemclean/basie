#pragma once
#include <cmath>
namespace daisysp {
struct Oscillator {
    enum { WAVE_SIN };
    float frequency = 0;
    void Init(float) {}
    void SetAmp(float) {}
    void SetFreq(float f) { frequency = f; }
    void SetWaveform(int) {}
    float Process() { return frequency / 1000.f; }
};
inline float mtof(float note) { return 440.f * std::pow(2.f, (note - 69.f) / 12.f); }
}
