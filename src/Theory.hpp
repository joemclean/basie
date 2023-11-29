#ifndef THEORY_H
#define THEORY_H

#include <array>
#include <string>
#include <vector>

struct Chord 
{
    std::array<int, 7> tones;
    std::string displayName;
    std::array<float, 12> chordScale;
    Chord(std::array<int, 7> tones, std::string displayName, std::array<float, 12> chordScale);
    ~Chord();
};

struct ChordScale
{
    std::array<int, 12> tones;
};

extern std::array<Chord *, 14> chordList;
extern std::array<std::string, 12> noteDisplayNames;


#endif // CHORD_THEORY_H