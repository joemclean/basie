#ifndef QUANTIZER_H
#define QUANTIZER_H

#include <vector>

using std::vector;
using std::pair;

pair<float, int> quantizeToScale(
  const float& noteInVoltage, 
  const float& chordRootOffsetVoltage, 
  const array<float, 12>& targetScale, 
  const float& jazzAmount);

#endif