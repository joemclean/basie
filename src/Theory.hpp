#ifndef THEORY_H
#define THEORY_H

#include <array>
#include <string>

using std::string;
using std::array;

namespace Theory {
  struct Chord {
    array<int, 7> tones;
    string displayName;
    array<float, 12> chordScale;

    Chord(array<int, 7> tones, string displayName, array<float, 12> chordScale);

    ~Chord();
  };

  struct ChordScale {
    array<int, 12> tones;
  };

  extern array<Chord *, 14> chordList;
  extern array<string, 12> noteDisplayNames;
}

#endif