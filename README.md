# Basie

Basie runs on the **Daisy Patch**. It is a two-channel CV quantizer following a chord progression stored on an SD card, with four-note MIDI chords and three audio oscillators.

## Build and test

Application code uses **C++17**. Firmware builds use **Arm GNU Toolchain 13.3.Rel1 (GCC 13.3.1)**, **libDaisy v8.1.0**, and **DaisySP V1.0.0**. Exact SDK commits are in [dependencies.json](dependencies.json).

Install [Arm GNU Toolchain 13.3.Rel1](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads), Git, Make, and Python 3. A compiler extracted into `vendor/toolchain/` is detected automatically. Otherwise add its `bin` directory to PATH, or pass `GCC_PATH=/absolute/path/to/toolchain/bin` to the firmware Make commands.

```sh
make deps                  # Fetch the pinned SDKs into ignored vendor/ directories.
make -j4 libs              # Build libDaisy and DaisySP.
make -j4                   # Build build/Basie.elf, .bin, and .hex.
make test                  # Desktop tests; only Python 3 and a C++17 compiler needed.
make test-sanitize         # Same tests with address/undefined-behavior sanitizers.
```

Use `HOST_CXX=clang++ make test-sanitize` to choose the desktop compiler. SDK directories can be overridden with `LIBDAISY_DIR=... DAISYSP_DIR=...`; reproducible builds should use the pinned commits. VS Code defaults to the project-local compiler path; update `.vscode/c_cpp_properties.json` if your compiler is elsewhere. The standard build targets internal flash (`APP_TYPE=BOOT_NONE`); alternate bootloader layouts are not validated. Check the firmware's flash/RAM usage printed by the linker before flashing.

For USB flashing, put the Daisy into its ROM DFU bootloader and run `make program-dfu` with `dfu-util` installed. For an ST-Link debugger, install OpenOCD and use `make program`. These commands write the connected device. The desktop tests do not flash hardware.

GitHub Actions runs sanitizer tests on Linux and builds firmware using the pinned compiler and SDKs. See [the changes explained](docs/changes-explained.md) for the design and C++ concepts, and [hardware checks](docs/hardware-checks.md) before relying on a new firmware build.

## Song files

Put `.txt` files in the SD card's root directory. Extensions are case-insensitive; files are listed alphabetically. Example:

```text
C maj7 | A min7 | D min7 | G 7 |
```

Each chord is a **root**, whitespace, and a **type**. Spaces, tabs, and line breaks count as whitespace. Bars separate chords, not measures: **one clock advances one chord**. Repeat a chord to hold it for multiple clocks. Leading/trailing bars and empty separators are ignored for compatibility. Comments, durations, and slash chords are not supported.

Roots: `C Db D Eb E F Gb G Ab A Bb B`, with `C# D# F# G# A#` accepted as equivalent sharp spellings. The display uses flats.

Types: `maj7 min7 6 min6 7 halfdim7 dim7 7b9 7#9 maj7#11 7#11 7b13 aug`.

Limits: **4095 bytes per file**, **1024 chords**, **256 song files**. Oversized, empty, unreadable, and invalid songs show an error. Invalid musical symbols are rejected. A failed load leaves the previous song playing. See [examples/progression.txt](examples/progression.txt).

On boot, Basie tries the first file. Without a valid song, audio and CV outputs stay at zero and MIDI is silent. Dismiss an error with a click. In an empty file browser, click to retry mounting/scanning the card. Automatic hot-plug detection is not implemented; restart after replacing a card that already had a populated file list.

## Controls and playback

| Control | Action |
| --- | --- |
| Gate In 1 | Advance one chord; wrap at the end |
| Gate In 2 | Reset immediately to the first chord |
| CV 1, CV 2 | Pitch inputs, nominally 0–5 V at 1 V/octave |
| CV 3, CV 4 | Jazz amount for each quantizer |
| CV Out 1, CV Out 2 | Nearest enabled pitch within 0–5 V |
| Hold encoder and turn | Switch between song and file views |
| Turn encoder in file view | Move the file cursor |
| Click encoder in file view | Load selected song; click again to dismiss a load error |

Loading a valid song applies chord one immediately. The next clock advances to chord two. Reset applies chord one to CV, MIDI, and audio. If reset and clock are sampled together, reset wins. Clock/reset still retrigger a one-chord song.

MIDI uses **channel 1**, velocity 100, with four notes: root, third, fifth, and sixth/seventh. The base root is MIDI note `36 + root pitch class`; the upper three tones are raised an octave. Chord extensions affect the quantization scale but are not additional MIDI voices. Each sounding note gets a matching note-off before the next voicing. Identical successive chords retrigger. If clocks exceed MIDI's transmission capacity, intermediate MIDI voicings are coalesced to the latest requested chord; CV/audio still process each sampled clock. SD file operations can also defer foreground MIDI servicing while CV/audio continue.

Audio outputs 1–3 play the first three voiced notes as sine waves (0, 0.1, and 0.2 Hz downward detuning). Output 4 mixes them. No song means silence.

Jazz preserves the original weighted-note policy and 0.1 tolerance. Increasing it only adds candidate pitches; at 0.9 and above the scale is chromatic. The `min7` scale is corrected to natural minor, including its minor seventh. Other musical weights are preserved rather than retuned. The display uses exactly the same enabled-note mask as the quantizer.

Quantization considers adjacent octaves. Equal-distance ties choose the lower note. Notes outside 0–5 V are excluded, and the displayed octave comes from the selected output note. DAC codes use the original nominal 819.2 codes/V, rounded and clamped to 0–4095; accurate 1 V/oct tracking still requires hardware measurement/calibration.

## Timing

At 48 kHz with 24-sample blocks, controls and gates are processed every **0.5 ms** in the audio callback. Use gate pulses at least **1 ms** wide, with at least 1 ms low between pulses; shorter pulses are not guaranteed to be captured. Actual worst-case timing needs hardware verification. The callback does not parse text, allocate memory, render the screen, access SD files, or wait for MIDI transmission. The OLED updates at approximately 30 Hz. MIDI transmission uses a persistent DMA buffer and recovers from reported transfer errors by silencing channel 1 before retrying.

## Author and attribution

Joe McLean. The project began from the Electro-Smith [Daisy Patch Sequencer example](https://github.com/electro-smith/DaisyExamples/tree/master/patch/Sequencer) and uses libDaisy/DaisySP; their licenses remain in the SDK repositories.
