#include "display.hpp"
#include "theory.hpp"

#include "daisysp.h"
#include "daisy_patch.h"

using namespace daisy;
using namespace daisysp;

extern DaisyPatch patch;

namespace Display {

  int fileListPageIndex = 0;

  void drawKeyboard(
    const array<float, 12>& targetScale,
    const float& jazzAmount, 
    const int& chordRootIndex,
    const int& targetNoteIndex,
    const int& x_origin,
    const int& y_origin
  ) {
    bool shouldFillKey = false;
    int whiteKeyIndex = 0;
    int blackKeyIndex = 0;

    int keyHeight = 14;
    int keyWidth = 6;
    int blackKeyOffset = 3;
    int keyGap = 2;

    for (size_t i = 0; i < targetScale.size(); i++) {
      int targetIndex = (i + 12 - chordRootIndex) % 12;

      // TODO weirdly inverted?
      // TODO should be set outside (maybe as an array of avil notes)
      shouldFillKey = false;
      if (targetScale[targetIndex] >= (1 - jazzAmount)) {
        shouldFillKey = true;
      }
      // White keys
      if (i == 0 || i == 2 || i == 4 || i == 5 || i == 7 || i == 9 || i == 11) {
        patch.display.DrawRect(
          x_origin + ((whiteKeyIndex * keyWidth) + (keyGap * whiteKeyIndex)), 
          y_origin + keyHeight + keyGap, 
          x_origin + ((whiteKeyIndex + 1) * keyWidth) + (keyGap * whiteKeyIndex), 
          y_origin + (2 * keyHeight) + keyGap, 
          true, 
          shouldFillKey
        );
        if (i == (targetNoteIndex + chordRootIndex) % 12) {
          patch.display.DrawCircle(
            x_origin + (whiteKeyIndex * keyWidth) + (keyGap * whiteKeyIndex) + (keyWidth / 2), 
            y_origin + keyHeight + keyGap + (keyHeight / keyGap), 
            2, 
            false
          );
        }
        whiteKeyIndex++;
        // Black keys
      } else {
        patch.display.DrawRect(
          x_origin + ((blackKeyIndex * keyWidth) + blackKeyOffset + (keyGap * blackKeyIndex)), 
          y_origin,
          x_origin + ((blackKeyIndex + 1) * keyWidth) + blackKeyOffset + (keyGap * blackKeyIndex), 
          y_origin + keyHeight, 
          true, 
          shouldFillKey
        );
        if (i == (targetNoteIndex + chordRootIndex) % 12) {
          patch.display.DrawCircle(
            x_origin + (blackKeyIndex * keyWidth) + blackKeyOffset + (keyGap * blackKeyIndex) + (keyWidth / 2), 
            y_origin + keyHeight / 2, 
            2, 
            false
          );
        }
        blackKeyIndex++;
      }
      if (i == 4) {
        blackKeyIndex++;
      }
    }
  }

  void renderSongView(
    const string& songName,
    const string& targetChord,
    const int& playhead,
    const int& songLength,
    const float& jazzAmountCh1, 
    const float& jazzAmountCh2,
    const int& chordRootIndex,
    array<float, 12>& targetScale,
    const int& note1index,
    const int& note2index
  ) {
    string str = songName;
    char* cstr = &str[0];

    patch.display.SetCursor(2, 0);
    patch.display.WriteString(cstr, Font_6x8, true);

    patch.display.SetCursor(2, 10);
    str = targetChord;
    patch.display.WriteString(cstr, Font_6x8, true);

    patch.display.SetCursor(2, 20);
    str = "Chord " + std::to_string(playhead + 1) + "/" + std::to_string(songLength);
    patch.display.WriteString(cstr, Font_6x8, true);

    patch.display.SetCursor(2, 30);

    Display::drawKeyboard(targetScale, jazzAmountCh1, chordRootIndex, note1index, 2, 32);
    Display::drawKeyboard(targetScale, jazzAmountCh2, chordRootIndex, note2index, 72, 32);
  }

  void renderFileBrowser(
    const std::vector<std::string>& fileList,
    const int& loadedFileIndex,
    const int& fileListCursor
  ) {
    string headerStr = "File browser";
    patch.display.SetCursor(0, 0);
    patch.display.WriteString(headerStr.c_str(), Font_6x8, true);

    int filesPerPage = 5;
    fileListPageIndex = (int)fileListCursor / filesPerPage ;
    int lowerPageBound = fileListPageIndex * filesPerPage ;
    int fileListLength = static_cast<int>(fileList.size());

    int upperPageBound = (fileListPageIndex + 1) * filesPerPage;
    if (upperPageBound > fileListLength) {
      upperPageBound = fileListLength;
    }

    for (int i = lowerPageBound; i < upperPageBound; i++ ) {
      patch.display.SetCursor(0, (i + 1 - (filesPerPage * fileListPageIndex)) * 10);
      string fileStr;
      fileListCursor == i ? fileStr += ">" : fileStr += " ";
      loadedFileIndex == i ? fileStr += "*" : fileStr += " ";
      fileStr += fileList[i];
      patch.display.WriteString(fileStr.c_str(), Font_6x8, true);
    }
  }
}
