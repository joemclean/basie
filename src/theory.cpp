#include "theory.hpp"
#include <algorithm>
#include <cmath>
#include <iterator>

namespace Theory {
namespace {
// Preserve the jazz ordering; zero-weight notes enter at jazz 0.9.
constexpr Scale ionian       {1, 0, .1f, 0, .6f, .2f, 0, .7f, 0, .3f, 0, .5f};
constexpr Scale dorian       {1, 0, .3f, .6f, 0, .2f, 0, .7f, 0, .1f, .7f, 0};
constexpr Scale lydian       {1, 0, .1f, 0, .6f, 0, .7f, .7f, 0, .3f, 0, .6f};
constexpr Scale lydianDom    {1, 0, .1f, 0, .5f, 0, .7f, .7f, 0, .3f, .6f, 0};
constexpr Scale mixolydian   {1, 0, .1f, 0, .6f, .2f, 0, .7f, 0, .3f, .5f, 0};
constexpr Scale mixolydianB2 {1, .7f, 0, 0, .5f, .2f, 0, .7f, 0, .3f, .6f, 0};
constexpr Scale mixolydianB13{1, 0, .1f, 0, .5f, 1, 0, .8f, .7f, 0, .6f, 0};
constexpr Scale aeolian      {1, 0, .1f, .6f, 0, .2f, 0, .7f, .3f, 0, .5f, 0};
constexpr Scale locrian      {1, .1f, 0, .5f, 0, .2f, .7f, 0, .3f, 0, .5f, 0};
constexpr Scale diminished   {1, 0, .6f, .8f, 0, .6f, .8f, 0, .6f, .8f, 0, .6f};
constexpr Scale altered      {1, .3f, 0, .5f, .1f, 0, .8f, 0, .4f, 0, .6f, 0};
constexpr Scale augmented    {1, 0, .2f, 0, .5f, 0, 0, .2f, .8f, 0, 0, .2f};
constexpr std::array<const char*, 12> sharps{
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
constexpr float jazzTolerance = .1f;
}
// The compiler counts these entries; there is no unused fourteenth slot.
constexpr Chord chords[] = {
    {{0, 4, 7, 11, 14, 17, 21}, "maj7", ionian},
    {{0, 3, 7, 10, 14, 17, 20}, "min7", aeolian},
    {{0, 4, 7, 9, 14, 17, 21}, "6", ionian},
    {{0, 3, 7, 9, 14, 17, 21}, "min6", dorian},
    {{0, 4, 7, 10, 14, 17, 21}, "7", mixolydian},
    {{0, 3, 6, 10, 14, 17, 21}, "halfdim7", locrian},
    {{0, 3, 6, 9, 12, 15, 21}, "dim7", diminished},
    {{0, 4, 7, 10, 13, 17, 21}, "7b9", mixolydianB2},
    {{0, 4, 7, 10, 15, 17, 21}, "7#9", altered},
    {{0, 4, 7, 11, 14, 18, 21}, "maj7#11", lydian},
    {{0, 4, 7, 10, 14, 18, 21}, "7#11", lydianDom},
    {{0, 4, 7, 10, 14, 17, 20}, "7b13", mixolydianB13},
    {{0, 4, 8, 10, 14, 17, 21}, "aug", augmented},
};
const std::size_t chordCount = std::size(chords);
constexpr std::array<const char*, 12> noteNames{
    "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
int findRoot(std::string_view name) {
    for (std::size_t i = 0; i < noteNames.size(); ++i)
        if (name == noteNames[i] || name == sharps[i]) return static_cast<int>(i);
    return -1;
}
int findChord(std::string_view name) {
    for (std::size_t i = 0; i < chordCount; ++i)
        if (name == chords[i].name) return static_cast<int>(i);
    return -1;
}
NoteMask activeNotes(int root, const Scale& scale, float jazz) {
    if (!std::isfinite(jazz)) jazz = 0;
    jazz = std::clamp(jazz, 0.f, 1.f);
    root = (root % 12 + 12) % 12;
    NoteMask notes = 0;
    for (int degree = 0; degree < 12; ++degree)
        if (scale[degree] + jazzTolerance + 1e-6f >= 1.f - jazz)
            notes |= static_cast<NoteMask>(1u << ((root + degree) % 12));
    return notes;
}
bool contains(NoteMask notes, int pitchClass) {
    return pitchClass >= 0 && pitchClass < 12 && (notes & (1u << pitchClass)) != 0;
}
} // namespace Theory
