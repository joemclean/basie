#ifndef DISPLAY_H
#define DISPLAY_H

#include <array>

namespace Display {
  void drawKeyboard(
    const std::array<float, 12>& targetScale, 
    size_t& targetScaleSize, 
    const float& jazzAmount, 
    const int& chordRootIndex,
    const int& x_origin,
    const int& y_origin
  );
}

#endif