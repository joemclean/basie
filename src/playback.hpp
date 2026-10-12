#pragma once
#include "song.hpp"
#include <cstddef>
#include <cstdint>

class Playback {
  public:
    // Caller keeps this validated, immutable song alive until the next install.
    bool install(const Songs::Song& song);
    void process(bool clock, bool reset);
    const Songs::Event* current() const;
    std::size_t playhead() const { return playhead_; }
    std::size_t length() const { return song_ ? song_->events.size() : 0; }
    std::uint32_t revision() const { return revision_; }
  private:
    const Songs::Song* song_ = nullptr;
    std::size_t playhead_ = 0;
    std::uint32_t revision_ = 0;
};
