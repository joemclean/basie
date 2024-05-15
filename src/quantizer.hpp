#ifndef QUANTIZER_H
#define QUANTIZER_H

#include <array>

namespace Quantizer {
  std::pair<float, int> quantizeToScale(
    const float noteInVoltage, 
    const int chordRootIndex, 
    const std::array<float, 12>& targetScale, 
    const float jazzAmount
  );
}

#endif