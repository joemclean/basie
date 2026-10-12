#include "MIDI.hpp"
#include "theory.hpp"
#include <algorithm>

namespace MIDI {
Voicing voiceChord(const Songs::Event& event) {
    if (event.root >= 12 || event.chord >= Theory::chordCount) return {};
    Voicing result;
    result.count = voiceCount;
    const auto& chord = Theory::chords[event.chord];
    for (std::size_t i = 0; i < voiceCount; ++i)
        result.notes[i] = static_cast<std::uint8_t>(36 + event.root + chord.tones[i] + (i == 0 ? 0 : 12));
    return result;
}
Packet VoiceTracker::transition(const Voicing& next) const {
    Packet packet;
    const auto append = [&packet](std::uint8_t status, std::uint8_t note, std::uint8_t velocity) {
        packet.bytes[packet.size++] = status;
        packet.bytes[packet.size++] = note & 0x7f;
        packet.bytes[packet.size++] = velocity;
    };
    for (std::size_t i = 0; i < std::min(active_.count, voiceCount); ++i)
        append(static_cast<std::uint8_t>(0x80 | (active_.channel & 0x0f)), active_.notes[i], 0);
    for (std::size_t i = 0; i < std::min(next.count, voiceCount); ++i)
        append(static_cast<std::uint8_t>(0x90 | (next.channel & 0x0f)), next.notes[i], 100);
    return packet;
}
} // namespace MIDI
