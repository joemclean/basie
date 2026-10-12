#include "controls.hpp"
#include "MIDI.hpp"
#include "playback.hpp"
#include "quantizer.hpp"
#include "song.hpp"
#include "theory.hpp"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string>

namespace {
unsigned checks = 0;
void check(bool condition, const char* expression, int line) {
    ++checks;
    if (!condition) { std::cerr << "FAILED line " << line << ": " << expression << '\n'; std::exit(1); }
}
#define CHECK(x) check(static_cast<bool>(x), #x, __LINE__)
bool near(float a, float b) { return std::abs(a - b) < .00001f; }

void parsing() {
    for (const std::string text : {"", "  \n\t", "|||", "C", "Cmaj7", "C nonsense", "X maj7", "C maj7 extra", "C maj7 | D"}) {
        auto result = Songs::parse(text);
        CHECK(!result);
        CHECK(!result.error.empty());
        CHECK(result.song.events.empty());
    }
    auto good = Songs::parse(" | C maj7 | Bb\t halfdim7 | F# maj7#11 |\r\n");
    CHECK(good);
    CHECK(good.song.events.size() == 3);
    CHECK(good.song.events[1].root == 10);
    CHECK(good.song.events[2].root == 6);
    CHECK(Theory::chordCount == 13);
    for (std::size_t chord = 0; chord < Theory::chordCount; ++chord) {
        auto result = Songs::parse(std::string("Bb ") + Theory::chords[chord].name);
        CHECK(result);
        CHECK(result.song.events[0].chord == chord);
    }
    auto maxSize = Songs::parse(std::string(Songs::maxFileBytes - 3, ' ') + "C 6");
    CHECK(maxSize);
    CHECK(!Songs::parse(std::string(Songs::maxFileBytes + 1, ' ')));
    CHECK(!Songs::parse(std::string("C maj7\0|D min7", 14)));
    CHECK(Theory::findRoot("C#") == Theory::findRoot("Db"));
    // Deterministic malformed-input coverage, including arbitrary byte values.
    std::mt19937 random(1234);
    for (int sample = 0; sample < 3000; ++sample) {
        std::string input(random() % 128, ' ');
        for (auto& c : input) c = static_cast<char>(random() % 256);
        const auto result = Songs::parse(input);
        if (result) for (auto event : result.song.events) {
            CHECK(event.root < 12);
            CHECK(event.chord < Theory::chordCount);
        }
        else CHECK(!result.error.empty());
    }
}

void quantization() {
    constexpr Theory::NoteMask cOnly = 1, bOnly = 1 << 11, chromatic = 0xfff;
    CHECK(near(Quantizer::quantize(.99f, cOnly).voltage, 1.f));
    CHECK(near(Quantizer::quantize(.99f, chromatic).voltage, 1.f));
    CHECK(near(Quantizer::quantize(1.01f, bOnly).voltage, 11.f / 12.f));
    CHECK(near(Quantizer::quantize(.5f, cOnly).voltage, 0.f));
    CHECK(near(Quantizer::quantize(5.f, bOnly).voltage, 59.f / 12.f));
    CHECK(Quantizer::quantize(5.f, cOnly).octave == 5);
    CHECK(Quantizer::quantize(.99f, cOnly).octave == 1);
    CHECK(Quantizer::quantize(2.f, 0).voltage == 0.f);
    CHECK(Quantizer::dacCode(5.f) == 4095);
    CHECK(Quantizer::dacCode(10.f) == 4095);
    CHECK(Quantizer::dacCode(-1.f) == 0);
    CHECK(Quantizer::dacCode(std::numeric_limits<float>::quiet_NaN()) == 0);
    CHECK(Quantizer::quantize(std::numeric_limits<float>::infinity(), cOnly).voltage == 0);
    const auto& minor = Theory::chords[Theory::findChord("min7")];
    const auto minorNotes = Theory::activeNotes(0, minor.scale, .5f);
    CHECK(Theory::contains(minorNotes, 10));
    CHECK(!Theory::contains(minorNotes, 11));
    CHECK(Quantizer::quantize(10.f / 12.f, minorNotes).pitchClass == 10);
    for (std::size_t chord = 0; chord < Theory::chordCount; ++chord)
        for (int root = 0; root < 12; ++root) {
            Theory::NoteMask previousMask = 0;
            for (int jazz = 0; jazz <= 10; ++jazz) {
                const auto notes = Theory::activeNotes(root, Theory::chords[chord].scale, jazz / 10.f);
                CHECK(Theory::contains(notes, root));
                CHECK((notes & previousMask) == previousMask); // More jazz never removes notes.
                previousMask = notes;
                float previousVoltage = -1.f;
                for (int step = 0; step <= 120; ++step) {
                    const float input = step / 24.f;
                    const auto result = Quantizer::quantize(input, notes);
                    CHECK(result.voltage >= previousVoltage);
                    previousVoltage = result.voltage;
                    CHECK(result.voltage >= 0 && result.voltage <= 5);
                    CHECK(Theory::contains(notes, result.pitchClass));
                    CHECK(near(result.voltage, result.octave + result.pitchClass / 12.f));
                    // No other legal output in the full hardware range may be closer.
                    for (int pitch = 0; pitch <= 60; ++pitch)
                        if (Theory::contains(notes, pitch % 12))
                            CHECK(std::abs(result.voltage - input) <= std::abs(pitch / 12.f - input) + .00001f);
                }
            }
        }
}

void playback() {
    Playback player;
    player.process(true, true);
    CHECK(player.current() == nullptr);
    CHECK(player.revision() == 0);
    auto song = Songs::parse("C maj7 | D min7 | G 7");
    CHECK(player.install(song.song));
    CHECK(player.playhead() == 0);
    CHECK(player.current()->root == 0);
    const auto loadedRevision = player.revision();
    player.process(true, false);
    CHECK(player.playhead() == 1);
    CHECK(player.revision() != loadedRevision);
    player.process(true, true);
    CHECK(player.playhead() == 0); // Reset takes precedence over simultaneous clock.
    player.process(true, false); player.process(true, false); player.process(true, false);
    CHECK(player.playhead() == 0);
    Songs::Song invalid;
    CHECK(!player.install(invalid));
    CHECK(player.length() == 3);
    auto replacement = Songs::parse("F 6");
    CHECK(player.install(replacement.song));
    CHECK(player.current()->root == 5);
    const auto revision = player.revision();
    player.process(true, false);
    CHECK(player.revision() != revision); // A one-chord song still retriggers.
}

void midi() {
    MIDI::VoiceTracker tracker;
    std::array<int, 128> sounding{};
    auto consume = [&](const MIDI::Packet& packet) {
        CHECK(packet.size <= packet.bytes.size());
        CHECK(packet.size % 3 == 0);
        for (std::size_t i = 0; i < packet.size; i += 3) {
            const auto note = packet.bytes[i + 1];
            CHECK(note < 128);
            if (packet.bytes[i] == 0x90) { CHECK(sounding[note] == 0); ++sounding[note]; }
            else { CHECK(packet.bytes[i] == 0x80); CHECK(sounding[note] == 1); --sounding[note]; }
        }
    };
    const auto c = MIDI::voiceChord({0, 0});
    CHECK((c.notes == std::array<std::uint8_t, 4>{36, 52, 55, 59}));
    CHECK(tracker.transition({}).size == 0);
    for (std::size_t chord = 0; chord < Theory::chordCount; ++chord)
        for (int root = 0; root < 12; ++root) {
            const auto next = MIDI::voiceChord({static_cast<std::uint8_t>(root), static_cast<std::uint8_t>(chord)});
            consume(tracker.transition(next));
            tracker.commit(next);
            int total = 0;
            for (int n : sounding) total += n;
            CHECK(total == 4);
        }
    consume(tracker.transition({})); tracker.commit({});
    CHECK(tracker.transition({}).size == 0);
    for (int n : sounding) CHECK(n == 0);
}

void encoder() {
    Controls::EncoderActions input;
    Controls::Action event;
    input.process(1, true, false);
    input.process(0, false, true);
    CHECK(input.pop(event) && event.kind == Controls::Kind::ToggleTab);
    CHECK(!input.pop(event)); // Releasing after a tab change must not load a song.
    input.process(-1, false, false);
    input.process(0, false, true);
    CHECK(input.pop(event) && event.kind == Controls::Kind::Scroll && event.increment == -1);
    CHECK(input.pop(event) && event.kind == Controls::Kind::Select);
    for (int i = 0; i < 100; ++i) input.process(1, false, false);
    CHECK(input.takeOverflow()); CHECK(!input.takeOverflow());
    int count = 0;
    while (input.pop(event)) ++count;
    CHECK(count == 32);
    input.process(0, false, true);
    CHECK(input.pop(event) && event.kind == Controls::Kind::Select);
}
}
int main() {
    parsing(); quantization(); playback(); midi(); encoder();
    std::cout << "Core tests passed (" << checks << " checks)\n";
}
