// Include the actual application's callback and UI path in this translation
// unit, replacing only hardware APIs. Firmware main is compiled but not run.
#define main basieFirmwareMain
#include "../Basie.cpp"
#undef main
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
unsigned checks = 0;
void check(bool pass, const char* expression, int line) {
    ++checks;
    if (!pass) { std::cerr << "FAILED line " << line << ": " << expression << '\n'; std::exit(1); }
}
#define CHECK(x) check(static_cast<bool>(x), #x, __LINE__)
void tick() {
    float samples[4][24]{};
    float* outputs[]{samples[0], samples[1], samples[2], samples[3]};
    audioCallback(nullptr, outputs, 24);
}
void storage() {
    SDHandler::Card card;
    FakeFs::mountFailure = true;
    CHECK(!card.init().empty());
    FakeFs::mountFailure = false;
    CHECK(card.init().empty());
    FakeFs::names = {"z.TXT", "b.txt", "ignore.bin", "a.Txt"};
    auto list = card.listSongs();
    CHECK((list.files == std::vector<std::string>{"a.Txt", "b.txt", "z.TXT"}));
    FakeFs::directoryReadFailure = true;
    list = card.listSongs();
    CHECK(!list.error.empty() && list.files.empty());
    FakeFs::directoryReadFailure = false;
    CHECK(!card.load("missing.txt"));
    FakeFs::files["big.txt"] = std::string(4094, ' ') + "C maj7";
    int closes = FakeFs::closedFiles;
    CHECK(!card.load("big.txt"));
    CHECK(FakeFs::closedFiles == closes + 1);
    FakeFs::sizeOverride = 1;
    CHECK(!card.load("big.txt")); // Bound actual reads even when reported size is stale.
    FakeFs::sizeOverride = -1;
    FakeFs::files["ok.txt"] = std::string(4000, ' ') + "Bb halfdim7";
    CHECK(card.load("ok.txt")); // Multiple read chunks, full supported chord name.
    FakeFs::readFailure = true;
    CHECK(!card.load("ok.txt"));
    FakeFs::readFailure = false;
    FakeFs::closeFailure = true;
    CHECK(!card.load("ok.txt"));
    FakeFs::closeFailure = false;
}
void application() {
    // No song: gates are harmless, CV stays zero, audio callback remains valid.
    app.patch.gate_input[0].trigger = true;
    app.patch.gate_input[1].trigger = true;
    tick();
    CHECK(!snapshot().playing);
    CHECK(app.patch.seed.dac.values[0] == 0);
    app.browser = true;
    FakeFs::names.clear();
    app.controls.process(0, false, true);
    processUi();
    CHECK(app.files.empty());
    FakeFs::files["first.txt"] = "C maj7 | D min7";
    app.files = {"first.txt", "bad.txt", "second.txt"};
    FakeFs::files["bad.txt"] = "C maj7 | nope";
    FakeFs::files["second.txt"] = "F 6";
    loadSong(0);
    tick();
    CHECK(snapshot().playing && snapshot().chord.root == 0);
    CHECK(snapshot().voicing.count == 4);
    CHECK(std::abs(app.oscillators[0].frequency - daisysp::mtof(36)) < .001f);
    app.patch.gate_input[0].trigger = true;
    tick();
    CHECK(snapshot().chord.root == 2);
    const auto previousRevision = snapshot().revision;
    loadSong(1);
    tick();
    CHECK(!app.error.empty());
    CHECK(app.filename == "first.txt");
    CHECK(snapshot().revision == previousRevision);
    CHECK(snapshot().chord.root == 2);
    app.patch.gate_input[1].trigger = true;
    tick();
    CHECK(snapshot().playhead == 0 && snapshot().chord.root == 0);
    CHECK(snapshot().voicing.notes[0] == 36);
    CHECK(std::abs(app.oscillators[0].frequency - daisysp::mtof(36)) < .001f);
    app.patch.knobs[0] = .198f; // 0.99V, C-only: round to C in the next octave.
    tick();
    CHECK(std::abs(snapshot().notes[0].voltage - 1.f) < .00001f);
    CHECK(snapshot().notes[0].octave == 1);
    CHECK(app.patch.seed.dac.values[0] == Quantizer::dacCode(1.f));
    loadSong(2);
    tick();
    CHECK(snapshot().chord.root == 5 && snapshot().playhead == 0);
    CHECK(snapshot().voicing.notes[0] == 41);
    // Both song slots are reused; playback must never retain freed vector data.
    for (int i = 0; i < 100; ++i) { loadSong(i % 2 == 0 ? 0 : 2); tick(); }
    Display::song(app.patch.display, std::string(200, 'x'), snapshot());
    CHECK(app.patch.display.lines[1] == "F 6");
    CHECK(app.patch.display.lines[2] == "Chord 1/1");
    Display::files(app.patch.display, {}, -1, 0);
    CHECK(app.patch.display.lines[1] == "No .txt songs");
    Display::error(app.patch.display, "Chord 1234: unknown chord type", true);
    CHECK(std::find(app.patch.display.lines.begin(), app.patch.display.lines.end(), "Song still playing") != app.patch.display.lines.end());
}
void transmission() {
    using Uart = daisy::UartHandler;
    MidiOutput output;
    CHECK(output.init());
    const auto first = MIDI::voiceChord({0, 0});
    const auto second = MIDI::voiceChord({2, 1});
    const auto latest = MIDI::voiceChord({5, 2});
    Uart::transmissions.clear();
    output.service(first, 1);
    CHECK(Uart::transmissions.size() == 1 && Uart::transmissions.back().size() == 12);
    output.service(second, 2);
    output.service(latest, 3);
    CHECK(Uart::transmissions.size() == 1); // In-flight buffer must not be overwritten.
    Uart::finish();
    output.service(latest, 3);
    CHECK(Uart::transmissions.size() == 2 && Uart::transmissions.back().size() == 24);
    CHECK(Uart::transmissions.back()[0] == 0x80 && Uart::transmissions.back()[1] == 36);
    CHECK(Uart::transmissions.back()[12] == 0x90 && Uart::transmissions.back()[13] == 41);
    Uart::finish(); output.service(latest, 3);
    CHECK(Uart::transmissions.size() == 2);
    Uart::reject = true;
    output.service(second, 4);
    CHECK(output.failed());
    Uart::reject = false;
    output.service(second, 4);
    CHECK(Uart::transmissions.back()[0] == 0xb0 && Uart::transmissions.back()[1] == 120);
    Uart::finish(Uart::Result::ERR);
    output.service(first, 5);
    CHECK(Uart::transmissions.back()[0] == 0xb0); // Partial transfer gets a panic before retry.
    Uart::finish(); output.service(first, 5);
    CHECK(!output.failed());
}
}
int main() {
    storage(); application(); transmission();
    std::cout << "Hardware-adapter/application tests passed (" << checks << " checks)\n";
}
