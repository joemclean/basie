#include "display.hpp"
#include "theory.hpp"

#include "daisysp.h"
#include "daisy_patch.h"

using namespace daisy;
using namespace daisysp;

extern DaisyPatch patch;

namespace Display {

  std::string displayLineOne = "";
  std::string displayLineTwo = "";
  std::string displayLineThree = "";

  void drawKeyboard(
    const array<float, 12>& targetScale,
    size_t& targetScaleSize, 
    const float& jazzAmount, 
    const int& chordRootIndex,
    const int& x_origin,
    const int& y_origin
  ) {
    bool shouldFill = false;
    int whiteKeyIndex = 0;
    int blackKeyIndex = 0;

    int keyHeight = 14;
    int keyWidth = 6;
    int blackKeyOffset = 3;
    int keyGap = 2;

    for (size_t i = 0; i < targetScaleSize; i++) {
      int targetIndex = (i + 12 - chordRootIndex) % 12;

      // TODO weirdly inverted?
      // TODO should be set outside (maybe as an array of avil notes)
      shouldFill = false;
      if (targetScale[targetIndex] >= (1 - jazzAmount)) {
        shouldFill = true;
      }
      // White keys
      if (i == 0 || i == 2 || i == 4 || i == 5 || i == 7 || i == 9 || i == 11) {
        patch.display.DrawRect(
          x_origin + (whiteKeyIndex * keyWidth), 
          y_origin + keyHeight + keyGap, 
          x_origin + (whiteKeyIndex + 1) * keyWidth, 
          y_origin + (2 * keyHeight) + keyGap, 
          true, 
          shouldFill
        );
        whiteKeyIndex++;
        // Black keys
      } else {
        patch.display.DrawRect(
          x_origin + (blackKeyIndex * keyWidth + blackKeyOffset), 
          y_origin,
          x_origin + ((blackKeyIndex + 1) * keyWidth) + blackKeyOffset, 
          y_origin + keyHeight, 
          true, 
          shouldFill
        );
        blackKeyIndex++;
      }
      if (i == 4) {
        blackKeyIndex++;
      }
    }
  }

  void renderSongView(
    const float& jazzAmountCh1, 
    const float& jazzAmountCh2,
    const int& chordRootIndex,
    array<float, 12>& targetScale
  ) {
    string str = displayLineOne;
    char* cstr = &str[0];

    patch.display.SetCursor(0, 0);
    patch.display.WriteString(cstr, Font_7x10, true);

    patch.display.SetCursor(0, 10);
    str = displayLineTwo;
    patch.display.WriteString(cstr, Font_7x10, true);

    patch.display.SetCursor(0, 20);
    str = displayLineThree;
    patch.display.WriteString(cstr, Font_7x10, true);

    patch.display.SetCursor(0, 30);

    size_t targetScaleSize = targetScale.size();

    int ch2XOffset = 50;

    Display::drawKeyboard(targetScale, targetScaleSize, jazzAmountCh1, chordRootIndex, 0, 30);
    Display::drawKeyboard(targetScale, targetScaleSize, jazzAmountCh2, chordRootIndex, ch2XOffset, 30);
  }
}
