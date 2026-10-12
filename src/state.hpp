#pragma once
#include "MIDI.hpp"
#include "quantizer.hpp"
#include "song.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

// A small value snapshot: it has no strings, vectors, or owning pointers.
struct PlaybackView {
    bool playing = false;
    Songs::Event chord{};
    std::size_t playhead = 0;
    std::size_t songLength = 0;
    std::uint32_t revision = 0;
    std::array<Theory::NoteMask, 2> activeNotes{};
    std::array<Quantizer::Note, 2> notes{};
    MIDI::Voicing voicing{};
};
