#pragma once
#include "state.hpp"
#include "daisy_patch.h"
#include <string>
#include <vector>

namespace Display {
using Oled = daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver>;
void song(Oled& display, const std::string& filename, const PlaybackView& view);
void files(Oled& display, const std::vector<std::string>& names, int loaded, int cursor);
void error(Oled& display, const std::string& message, bool playing);
} // namespace Display
