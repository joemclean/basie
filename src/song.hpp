#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Songs {
constexpr std::size_t maxFileBytes = 4095;
constexpr std::size_t maxChords = 1024;
struct Event { std::uint8_t root = 0; std::uint8_t chord = 0; };
struct Song { std::vector<Event> events; };
struct ParseResult {
    Song song;
    std::string error;
    explicit operator bool() const { return error.empty() && !song.events.empty(); }
};
ParseResult parse(std::string_view text);
} // namespace Songs
