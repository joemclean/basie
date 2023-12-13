#ifndef QUANTIZER_H
#define QUANTIZER_H

#include <array>

using std::pair;
using std::array;

pair<float, int> quantizeToScale(
  const float& noteInVoltage, 
  const float& chordRootOffsetVoltage, 
  const array<float, 12>& targetScale, 
  const float& jazzAmount);

#endif