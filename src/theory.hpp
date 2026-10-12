#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Theory {
using Scale = std::array<float, 12>;
using NoteMask = std::uint16_t;
// Literal-backed names and fixed arrays need no allocation or constructor.
struct Chord {
    std::array<int, 7> tones;
    const char* name;
    Scale scale;
};
extern const Chord chords[];
extern const std::size_t chordCount;
extern const std::array<const char*, 12> noteNames;
int findRoot(std::string_view name);
int findChord(std::string_view name);
NoteMask activeNotes(int root, const Scale& scale, float jazz);
bool contains(NoteMask notes, int pitchClass);
} // namespace Theory
