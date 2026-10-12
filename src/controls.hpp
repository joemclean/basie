#pragma once
#include <array>
#include <cstddef>

namespace Controls {
enum class Kind { Scroll, ToggleTab, Select };
struct Action { Kind kind = Kind::Scroll; int increment = 0; };

// Audio callback is the producer. Foreground pop/takeOverflow calls briefly
// exclude that interrupt. No allocations or blocking operations are involved.
class EncoderActions {
  public:
    void process(int increment, bool pressed, bool released) {
        if (increment != 0) {
            if (pressed) { push({Kind::ToggleTab, 0}); turnedWhileHeld_ = true; }
            else push({Kind::Scroll, increment});
        }
        if (released) {
            if (!turnedWhileHeld_) push({Kind::Select, 0});
            turnedWhileHeld_ = false;
        }
    }
    bool pop(Action& action) {
        if (count_ == 0) return false;
        action = actions_[read_];
        read_ = (read_ + 1) % actions_.size();
        --count_;
        return true;
    }
    bool takeOverflow() { const bool value = overflow_; overflow_ = false; return value; }
  private:
    void push(Action action) {
        if (count_ == actions_.size()) { overflow_ = true; return; }
        actions_[(read_ + count_) % actions_.size()] = action;
        ++count_;
    }
    std::array<Action, 32> actions_{};
    std::size_t read_ = 0;
    std::size_t count_ = 0;
    bool turnedWhileHeld_ = false;
    bool overflow_ = false;
};
} // namespace Controls
