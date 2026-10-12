#pragma once
#include "song.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace MIDI {
constexpr std::size_t voiceCount = 4;
struct Voicing {
    std::array<std::uint8_t, voiceCount> notes{};
    std::size_t count = 0;
    std::uint8_t channel = 0;
};
struct Packet {
    std::array<std::uint8_t, voiceCount * 6> bytes{};
    std::size_t size = 0;
};
Voicing voiceChord(const Songs::Event& event);
class VoiceTracker {
  public:
    // Build without mutation; commit only after transmission is accepted.
    Packet transition(const Voicing& next) const;
    void commit(const Voicing& next) { active_ = next; }
    void reset() { active_ = {}; }
  private:
    Voicing active_;
};
} // namespace MIDI
