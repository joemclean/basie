#pragma once
#include "daisy_core.h"
#include "fatfs.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
inline int Font_6x8 = 0;
namespace daisy {
struct SSD130x4WireSpi128x64Driver {};
template<typename Driver> struct OledDisplay {
    std::vector<std::string> lines;
    void Fill(bool) { lines.clear(); }
    void Update() {}
    void SetCursor(int, int) {}
    void WriteString(const char* text, int, bool) { lines.emplace_back(text); }
    void DrawRect(int, int, int, int, bool, bool) {}
    void DrawCircle(int, int, int, bool) {}
};
struct AudioHandle { using InputBuffer = const float* const*; using OutputBuffer = float**; };
struct DacHandle {
    enum class Channel { ONE, TWO };
    std::uint16_t values[2]{};
    void WriteValue(Channel c, std::uint16_t v) { values[c == Channel::ONE ? 0 : 1] = v; }
};
struct Encoder {
    int increment = 0; bool pressed = false, released = false;
    int Increment() { return increment; }
    bool Pressed() { return pressed; }
    bool FallingEdge() { return released; }
};
struct Gate { bool trigger = false; bool Trig() { const bool result = trigger; trigger = false; return result; } };
struct DaisyPatch {
    enum Ctrl { CTRL_1, CTRL_2, CTRL_3, CTRL_4 };
    OledDisplay<SSD130x4WireSpi128x64Driver> display;
    Encoder encoder;
    Gate gate_input[2];
    struct { DacHandle dac; } seed;
    float knobs[4]{};
    void Init() {}
    void SetAudioBlockSize(int) {}
    float AudioSampleRate() { return 48000; }
    void StartAdc() {}
    void StartAudio(void (*)(AudioHandle::InputBuffer, AudioHandle::OutputBuffer, std::size_t)) {}
    void ProcessAnalogControls() {}
    void ProcessDigitalControls() {}
    float GetKnobValue(Ctrl c) { return knobs[c]; }
    void DelayMs(unsigned) {}
};
struct System { static std::uint32_t GetNow() { return 0; } };
struct SdmmcHandler {
    enum class Result { OK, ERROR };
    struct Config { void Defaults() {} };
    Result Init(Config) { return Result::OK; }
};
struct FatFSInterface {
    enum Result { OK, ERR_GENERIC };
    struct Config { enum { MEDIA_SD }; };
    int filesystem = 0;
    Result Init(int) { return OK; }
    int& GetSDFileSystem() { return filesystem; }
    const char* GetSDPath() { return "0:/"; }
};
}
