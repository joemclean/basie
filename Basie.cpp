#include "daisy_patch.h"
#include "daisysp.h"
#include "util/scopedirqblocker.h"
#include "src/controls.hpp"
#include "src/display.hpp"
#include "src/midi_output.hpp"
#include "src/playback.hpp"
#include "src/sdhandler.hpp"
#include "src/state.hpp"
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace {
struct Application {
    daisy::DaisyPatch patch;
    SDHandler::Card card;
    MidiOutput midi;
    Playback playback;
    Controls::EncoderActions controls;
    PlaybackView view;
    std::array<daisysp::Oscillator, 3> oscillators;
    std::uint32_t audioRevision = 0;

    // Foreground-only storage. Playback reads one immutable song; the other
    // can be replaced without disturbing the callback or freeing its data.
    std::array<Songs::Song, 2> songs;
    int activeSong = 0;
    std::vector<std::string> files;
    std::string filename = "No song";
    std::string error;
    int cursor = 0;
    int loaded = -1;
    bool browser = false;
};
Application app;

void audioCallback(daisy::AudioHandle::InputBuffer,
                   daisy::AudioHandle::OutputBuffer output, std::size_t size) {
    auto& patch = app.patch;
    patch.ProcessAnalogControls();
    patch.ProcessDigitalControls();
    app.controls.process(patch.encoder.Increment(), patch.encoder.Pressed(), patch.encoder.FallingEdge());
    // Always sample both inputs; reset wins if both have an edge in this block.
    const bool clock = patch.gate_input[0].Trig();
    const bool reset = patch.gate_input[1].Trig();
    app.playback.process(clock, reset);
    const auto* event = app.playback.current();
    auto& view = app.view;
    view.playing = event != nullptr;
    view.playhead = app.playback.playhead();
    view.songLength = app.playback.length();
    view.revision = app.playback.revision();
    if (event) {
        view.chord = *event;
        if (app.audioRevision != view.revision) {
            view.voicing = MIDI::voiceChord(*event);
            for (std::size_t i = 0; i < app.oscillators.size(); ++i)
                app.oscillators[i].SetFreq(daisysp::mtof(view.voicing.notes[i]) - static_cast<float>(i) * .1f);
            app.audioRevision = view.revision;
        }
        const auto& chord = Theory::chords[event->chord];
        for (std::size_t channel = 0; channel < 2; ++channel) {
            const auto pitchControl = channel == 0 ? daisy::DaisyPatch::CTRL_1 : daisy::DaisyPatch::CTRL_2;
            const auto jazzControl = channel == 0 ? daisy::DaisyPatch::CTRL_3 : daisy::DaisyPatch::CTRL_4;
            view.activeNotes[channel] = Theory::activeNotes(event->root, chord.scale, patch.GetKnobValue(jazzControl));
            view.notes[channel] = Quantizer::quantize(patch.GetKnobValue(pitchControl) * 5.f, view.activeNotes[channel]);
        }
    }
    patch.seed.dac.WriteValue(daisy::DacHandle::Channel::ONE, Quantizer::dacCode(view.notes[0].voltage));
    patch.seed.dac.WriteValue(daisy::DacHandle::Channel::TWO, Quantizer::dacCode(view.notes[1].voltage));
    for (std::size_t sample = 0; sample < size; ++sample) {
        float mix = 0;
        for (std::size_t voice = 0; voice < app.oscillators.size(); ++voice) {
            const float signal = event ? app.oscillators[voice].Process() : 0.f;
            output[voice][sample] = signal;
            mix += signal * .25f;
        }
        output[3][sample] = mix;
    }
}

PlaybackView snapshot() {
    // RAII restores the previous interrupt state automatically on return.
    // Only copy a small value here: no allocation, file I/O, or display work.
    const daisy::ScopedIrqBlocker lock;
    return app.view;
}

void loadSong(int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= app.files.size()) return;
    auto candidate = app.card.load(app.files[index]);
    if (!candidate) { app.error = candidate.error; return; }
    const int slot = 1 - app.activeSong;
    app.songs[slot] = std::move(candidate.song);
    bool installed;
    {
        const daisy::ScopedIrqBlocker lock;
        installed = app.playback.install(app.songs[slot]);
    }
    if (!installed) { app.error = "Invalid song"; return; }
    app.activeSong = slot;
    app.filename = app.files[index];
    app.loaded = index;
    app.error.clear();
}

void rescan() {
    app.error = app.card.init();
    if (!app.error.empty()) return;
    auto listing = app.card.listSongs();
    if (!listing.error.empty()) { app.error = listing.error; return; }
    app.files = std::move(listing.files);
    app.cursor = 0;
    app.loaded = -1;
    for (std::size_t i = 0; i < app.files.size(); ++i)
        if (app.files[i] == app.filename) app.loaded = static_cast<int>(i);
}

void processUi() {
    Controls::Action action;
    for (int handled = 0; handled < 32; ++handled) {
        bool available;
        bool overflow;
        {
            const daisy::ScopedIrqBlocker lock;
            available = app.controls.pop(action);
            overflow = app.controls.takeOverflow();
        }
        if (overflow) app.error = "Controls busy; try again";
        if (!available) break;
        if (!app.error.empty()) {
            if (action.kind == Controls::Kind::Select) app.error.clear();
            continue;
        }
        if (action.kind == Controls::Kind::ToggleTab) app.browser = !app.browser;
        else if (action.kind == Controls::Kind::Scroll && app.browser && !app.files.empty()) {
            const int count = static_cast<int>(app.files.size());
            app.cursor = ((app.cursor + action.increment) % count + count) % count;
        } else if (action.kind == Controls::Kind::Select && app.browser) {
            if (app.files.empty()) rescan();
            else loadSong(app.cursor);
        }
    }
}
} // namespace

int main() {
    app.patch.Init();
    app.patch.SetAudioBlockSize(24); // 48 kHz / 24 = 2 kHz control and gate sampling.
    for (auto& oscillator : app.oscillators) {
        oscillator.Init(app.patch.AudioSampleRate());
        oscillator.SetAmp(.7f);
        oscillator.SetWaveform(daisysp::Oscillator::WAVE_SIN);
    }
    const bool midiReady = app.midi.init();
    rescan();
    if (app.error.empty() && !app.files.empty()) loadSong(0);
    if (!midiReady && app.error.empty()) app.error = "MIDI initialization failed";
    app.patch.StartAdc();
    app.patch.StartAudio(audioCallback);
    std::uint32_t lastDisplay = daisy::System::GetNow() - 33;
    while (true) {
        processUi();
        const auto view = snapshot();
        app.midi.service(view.voicing, view.revision);
        const auto now = daisy::System::GetNow();
        if (now - lastDisplay >= 33) {
            lastDisplay = now;
            if (!app.error.empty()) Display::error(app.patch.display, app.error, view.playing);
            else if (app.midi.failed()) Display::error(app.patch.display, "MIDI send failed; retrying", view.playing);
            else if (app.browser) Display::files(app.patch.display, app.files, app.loaded, app.cursor);
            else Display::song(app.patch.display, app.filename, view);
            app.patch.display.Update();
        }
        app.patch.DelayMs(1);
    }
}
