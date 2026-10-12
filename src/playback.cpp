#include "playback.hpp"
#include "theory.hpp"
bool Playback::install(const Songs::Song& song) {
    if (song.events.empty() || song.events.size() > Songs::maxChords) return false;
    for (const auto& event : song.events)
        if (event.root >= 12 || event.chord >= Theory::chordCount) return false;
    song_ = &song;
    playhead_ = 0;
    ++revision_;
    return true;
}
void Playback::process(bool clock, bool reset) {
    if (!song_) return;
    if (reset) { playhead_ = 0; ++revision_; }
    else if (clock) { playhead_ = (playhead_ + 1) % length(); ++revision_; }
}
const Songs::Event* Playback::current() const {
    return song_ ? &song_->events[playhead_] : nullptr;
}
