#ifndef DISPLAY_H
#define DISPLAY_H

#include <array>
#include <string>
#include <vector>

namespace Display {

  extern std::string displayLineOne;
  extern std::string displayLineTwo;
  extern std::string displayLineThree;
  extern std::string chordDisplay;

  extern int fileListCursor;

  void drawKeyboard(
    const std::array<float, 12>& targetScale, 
    size_t& targetScaleSize, 
    const float& jazzAmount, 
    const int& chordRootIndex,
    const int& x_origin,
    const int& y_origin
  );

  void renderSongView(
    const float& jazzAmountCh1, 
    const float& jazzAmountCh2,
    const int& chordRootIndex,
    std::array<float, 12>& targetScale
  );

  void renderFileBrowser(
    const std::vector<std::string>& fileList,
    const int& loadedFileIndex
  );
}

#endif