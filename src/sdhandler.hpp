#pragma once
#include "song.hpp"
#include "daisy_patch.h"
#include "fatfs.h"
#include <string>
#include <vector>

namespace SDHandler {
struct FileList {
    std::vector<std::string> files;
    std::string error;
};
class Card {
  public:
    std::string init();
    FileList listSongs();
    Songs::ParseResult load(const std::string& path);
  private:
    daisy::SdmmcHandler sd_;
    daisy::FatFSInterface filesystem_;
    // FatFs owns a sector buffer in FIL; keep it in AXI SRAM, not the DTCM stack.
    FIL file_{};
    bool initialized_ = false;
};
} // namespace SDHandler
