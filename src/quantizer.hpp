#ifndef QUANTIZER_H
#define QUANTIZER_H

#include <array>
#include <string>
#include <vector>

std::pair<float, int> quantizeToScale(
  const float& noteInVoltage, 
  const float& chordRootOffsetVoltage, 
  const std::array<float, 12>& targetScale, 
  const float& jazzAmount);

#endif