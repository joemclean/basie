#include "display.hpp"
#include "theory.hpp"
#include <algorithm>

namespace Display {
namespace {
void text(Oled& display, int x, int y, const char* value) {
    display.SetCursor(x, y);
    display.WriteString(value, Font_6x8, true);
}
void keyboard(Oled& display, Theory::NoteMask notes, int selected, int x) {
    int white = 0, black = 0;
    constexpr int width = 6, height = 14, gap = 2, y = 32;
    for (int pitch = 0; pitch < 12; ++pitch) {
        const bool isWhite = pitch == 0 || pitch == 2 || pitch == 4 || pitch == 5
                             || pitch == 7 || pitch == 9 || pitch == 11;
        const int left = x + (isWhite ? white : black) * (width + gap) + (isWhite ? 0 : 3);
        const int top = y + (isWhite ? height + gap : 0);
        const bool active = Theory::contains(notes, pitch);
        display.DrawRect(left, top, left + width, top + height, true, active);
        if (selected == pitch) display.DrawCircle(left + width / 2, top + height / 2, 2, !active);
        if (isWhite) ++white; else ++black;
        if (pitch == 4) ++black;
    }
}
}
void song(Oled& display, const std::string& filename, const PlaybackView& view) {
    display.Fill(false);
    text(display, 0, 0, filename.c_str());
    if (!view.playing) {
        text(display, 0, 16, "No song loaded");
        text(display, 0, 32, "Hold + turn: files");
        return;
    }
    const std::string chord = std::string(Theory::noteNames[view.chord.root]) + " "
                              + Theory::chords[view.chord.chord].name;
    text(display, 0, 10, chord.c_str());
    const std::string counter = "Chord " + std::to_string(view.playhead + 1) + "/"
                                + std::to_string(view.songLength);
    text(display, 0, 20, counter.c_str());
    keyboard(display, view.activeNotes[0], view.notes[0].pitchClass, 0);
    keyboard(display, view.activeNotes[1], view.notes[1].pitchClass, 73);
    for (int octave = 0; octave <= 5; ++octave) {
        display.DrawCircle(59, 35 + (5 - octave) * 5, view.notes[0].octave == octave ? 2 : 1, true);
        display.DrawCircle(68, 35 + (5 - octave) * 5, view.notes[1].octave == octave ? 2 : 1, true);
    }
}
void files(Oled& display, const std::vector<std::string>& names, int loaded, int cursor) {
    display.Fill(false);
    text(display, 0, 0, "File browser");
    if (names.empty()) {
        text(display, 0, 16, "No .txt songs");
        text(display, 0, 32, "Click to rescan SD");
        return;
    }
    cursor = std::clamp(cursor, 0, static_cast<int>(names.size()) - 1);
    const int first = (cursor / 5) * 5;
    const int last = std::min(first + 5, static_cast<int>(names.size()));
    for (int i = first; i < last; ++i) {
        const std::string line = std::string(i == cursor ? ">" : " ") + (i == loaded ? "*" : " ") + names[i];
        text(display, 0, (i - first + 1) * 10, line.c_str());
    }
}
void error(Oled& display, const std::string& message, bool playing) {
    display.Fill(false);
    text(display, 0, 0, "Unable to continue");
    // Wrap the complete bounded loader error across the screen.
    for (std::size_t start = 0; start < message.size() && start < 63; start += 21)
        text(display, 0, 12 + static_cast<int>(start / 21) * 10, message.substr(start, 21).c_str());
    text(display, 0, 44, playing ? "Song still playing" : "Outputs silent");
    text(display, 0, 56, "Click to dismiss");
}
} // namespace Display
