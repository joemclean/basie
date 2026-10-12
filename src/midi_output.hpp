#pragma once
#include "MIDI.hpp"
#include "per/uart.h"
#include <atomic>
#include <cstdint>

// One hardware MIDI output. service() runs only in the foreground; the DMA
// completion interrupt merely publishes a status. The latest chord wins if
// requests arrive faster than MIDI can transmit, without losing note-offs.
class MidiOutput {
  public:
    bool init();
    void service(const MIDI::Voicing& desired, std::uint32_t revision);
    bool failed() const { return failed_; }
  private:
    static void completed(void* context, daisy::UartHandler::Result result);
    daisy::UartHandler uart_;
    MIDI::VoiceTracker tracker_;
    std::atomic<int> transfer_{0}; // 0 idle, 1 busy, 2 complete, 3 failed
    std::uint32_t sentRevision_ = 0;
    bool hasSent_ = false;
    bool panic_ = false;
    bool failed_ = false;
    bool initialized_ = false;
};
static_assert(std::atomic<int>::is_always_lock_free, "DMA completion must not take a lock");
