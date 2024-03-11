#include "theory.hpp"

#include <string>
#include <array>

using std::string;
using std::array;

// Define chords as half steps above root

Theory::Chord::Chord(array<int, 7> tones, string displayName, array<float, 12> chordScale)
{
    this->tones = tones;
    this->displayName = displayName;
    this->chordScale = chordScale;
};

Theory::Chord::~Chord()
{
};

array<float, 12> ionianScale = {1, 0, 0.1, 0, 0.5, 0.3, 0, 0.7, 0, 0.3, 0, 0.5};
array<float, 12> dorianScale = {1, 0, 0.3, 0.5, 0, 0.3, 0, 0.7, 0, 0.1, 0.7, 0};
array<float, 12> phrygianScale = {1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0};
array<float, 12> lydianScale = {1, 0, 0.1, 0, 0.5, 0, 0.7, 0.7, 0, 0.3, 0, 0.6};
array<float, 12> lydianDominantScale = {1, 0, 0.1, 0, 0.5, 0, 0.7, 0.7, 0, 0.3, 0.6, 0};
array<float, 12> mixolydianScale = {1, 0, 0.1, 0, 0.5, 0.3, 0, 0.7, 0, 0.3, 0.5, 0};
array<float, 12> mixolydianFlat2Scale = {1, 0.7, 0, 0, 0.5, 0.2, 0, 0.7, 0, 0.2, 0.6, 0};
array<float, 12> mixolydianFlat13Scale = {1, 0, 0.1, 0, 0.5, 1, 0, 0.8, 0.7, 0, 0.6, 0};
array<float, 12> aeoleanScale = {1, 0, 0.1, 0.5, 0, 0.3, 0, 0.7, 0, 0.3, 0, 0.5};
array<float, 12> locrianScale = {1, 0.1, 0, 0.5, 0, 0.3, 0.7, 0, 0.3, 0, 0.5, 0};
array<float, 12> diminishedScale = {1, 0, 0.6, 0.8, 0, 0.6, 0.8, 0, 0.6, 0.8, 0, 0.6};
array<float, 12> alteredScale = {1, 0.3, 0, 0.5, 0.1, 0, 0.8, 0, 0.4, 0, 0.6, 0};
array<float, 12> augmentedScale = {1, 0, 0.2, 0, 0.5, 0, 0, 0.2, 0.8, 0, 0, 0.2};

// TODO not actually sure 13 is relevant for lots of these
// TODO more than one scale per chord
Theory::Chord *majorChord = new Theory::Chord({0, 4, 7, 11, 14, 17, 21}, "maj7", ionianScale);
Theory::Chord *minorChord = new Theory::Chord({0, 3, 7, 10, 14, 17, 21}, "min7", aeoleanScale);
Theory::Chord *major6Chord = new Theory::Chord({0, 4, 7, 9, 14, 17, 21}, "6", ionianScale);
Theory::Chord *minor6Chord = new Theory::Chord({0, 3, 7, 9, 14, 17, 21}, "min6", dorianScale);
Theory::Chord *dominantChord = new Theory::Chord({0, 4, 7, 10, 14, 17, 21}, "7", mixolydianScale);
Theory::Chord *halfDiminishedChord = new Theory::Chord({0, 3, 6, 10, 14, 17, 21}, "halfdim7", locrianScale);
Theory::Chord *diminishedChord = new Theory::Chord({0, 3, 6, 9, 12, 15, 21}, "dim7", diminishedScale);
Theory::Chord *flat9chord = new Theory::Chord({0, 4, 7, 10, 13, 17, 21}, "7b9", mixolydianFlat2Scale);
Theory::Chord *sharp9chord = new Theory::Chord({0, 4, 7, 10, 15, 17, 21}, "7#9", alteredScale);
Theory::Chord *sharp11chord = new Theory::Chord({0, 4, 7, 11, 14, 18, 21}, "maj7#11", lydianScale);
Theory::Chord *dominantSharp11chord = new Theory::Chord({0, 4, 7, 10, 14, 18, 21}, "7#11", lydianDominantScale);
Theory::Chord *flat13chord = new Theory::Chord({0, 4, 7, 10, 14, 17, 20}, "7b13", mixolydianFlat13Scale);
Theory::Chord *augmentedChord = new Theory::Chord({0, 4, 8, 10, 14, 17, 21}, "aug", augmentedScale);

// TODO OOPS DOESN'T WORK W SHARPS :(
array<string, 12> noteDisplayNames = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};

array<Theory::Chord *, 14> chordList = {majorChord, minorChord, major6Chord, minor6Chord, dominantChord, halfDiminishedChord, diminishedChord, flat9chord, sharp9chord, sharp11chord, dominantSharp11chord, flat13chord, augmentedChord };
