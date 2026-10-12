#include "song.hpp"
#include "theory.hpp"

namespace Songs {
namespace {
constexpr std::string_view whitespace = " \n\r\t\f\v";
std::string_view trim(std::string_view text) {
    const auto first = text.find_first_not_of(whitespace);
    if (first == std::string_view::npos) return {};
    return text.substr(first, text.find_last_not_of(whitespace) - first + 1);
}
ParseResult failure(std::size_t position, const char* reason) {
    return {{}, "Chord " + std::to_string(position) + ": " + reason};
}
}
ParseResult parse(std::string_view text) {
    if (text.size() > maxFileBytes) return {{}, "File exceeds 4095 bytes"};
    if (text.find('\0') != std::string_view::npos) return {{}, "File contains a NUL byte"};
    ParseResult result;
    while (!text.empty()) {
        const auto delimiter = text.find('|');
        const auto token = trim(text.substr(0, delimiter));
        // Preserve support for leading/trailing bars and empty separators.
        if (!token.empty()) {
            const auto position = result.song.events.size() + 1;
            const auto separator = token.find_first_of(whitespace);
            if (separator == std::string_view::npos) return failure(position, "expected root and type");
            const auto rootName = token.substr(0, separator);
            const auto typeName = trim(token.substr(separator));
            if (typeName.empty() || typeName.find_first_of(whitespace) != std::string_view::npos)
                return failure(position, "expected exactly root and type");
            const int root = Theory::findRoot(rootName);
            const int chord = Theory::findChord(typeName);
            if (root < 0) return failure(position, "unknown root");
            if (chord < 0) return failure(position, "unknown chord type");
            if (result.song.events.size() == maxChords) return failure(position, "too many chords");
            result.song.events.push_back({static_cast<std::uint8_t>(root), static_cast<std::uint8_t>(chord)});
        }
        if (delimiter == std::string_view::npos) break;
        text.remove_prefix(delimiter + 1);
    }
    if (result.song.events.empty()) result.error = "Song has no chords";
    return result;
}
} // namespace Songs
