#ifndef DISPLAY_H
#define DISPLAY_H

#include <array>
#include <string>
#include <vector>

namespace Display {

  void drawKeyboard(
    const std::array<float, 12>& targetScale, 
    const float& jazzAmount, 
    const int& chordRootIndex,
    const int& x_origin,
    const int& y_origin
  );

  void renderSongView(
    const std::string& songName,
    const std::string& targetChord,
    const int& playhead,
    const int& songLength,
    const float& jazzAmountCh1, 
    const float& jazzAmountCh2,
    const int& chordRootIndex,
    std::array<float, 12>& targetScale,
    const int& note1index,
    const int& note2index
  );

  void renderFileBrowser(
    const std::vector<std::string>& fileList,
    const int& loadedFileIndex,
    const int& fileListCursor
  );
}

#endif