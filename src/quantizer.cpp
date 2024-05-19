#include "quantizer.hpp"
#include <math.h>

#include <array>

using std::array;
using std::pair;
using std::make_pair;

namespace Quantizer {
  pair<float, int> quantizeToScale(
    const float noteInVoltage, 
    const int chordRootIndex, 
    const array<float, 12>& targetScale, 
    const float jazzAmount
  ) {
    float octaveVoltage, noteVoltage;
    float jazzThreshold = 1.0 - jazzAmount;
    // separate the whole voltage (octave) from fractional (note)
    octaveVoltage = floor(noteInVoltage);
    noteVoltage = noteInVoltage - octaveVoltage;
    int nearestScaleToneIndex = 0;
    float currentLowestNoteDistance = 1.0;
    bool quantizerStepIsActive = false;

    for (int i = 0; i < 12; i++) {
      int targetIndex = (i + (12 - chordRootIndex)) % 12;
      quantizerStepIsActive = (float)targetScale[targetIndex] + 0.1 >= jazzThreshold;
      if (quantizerStepIsActive) {
        float indexAsFloat = (float)i;
        float indexAsVoltage = (indexAsFloat/12);
        float noteDistance = abs(indexAsVoltage - noteVoltage);
        if (noteDistance < currentLowestNoteDistance) {
          currentLowestNoteDistance = noteDistance;
          nearestScaleToneIndex = i;
        }
      }
    }
    float baseNoteVoltage = (float)nearestScaleToneIndex / 12;
    float targetNoteVoltage = octaveVoltage + baseNoteVoltage;
    return make_pair(targetNoteVoltage, nearestScaleToneIndex);
  }
}