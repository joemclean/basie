# The changes, explained

The repair keeps Basie's existing modules and gives each one a smaller job. Read `src/theory.hpp`, `src/song.hpp`, and `src/playback.hpp` first, then `Basie.cpp` to see how the pieces fit together.

## 1. Fixed data does not need `new`

A chord definition is a record containing intervals, a name, and twelve weights. There are always thirteen supported definitions, and playback never edits them. The table is now a constant array of records, with its length inferred from its initializer.

Previously, each chord was allocated with `new`, and another global variable accessed the resulting pointers before their initialization was guaranteed. C++ does not generally guarantee dynamic initialization order across source files. An array with one extra pointer also silently filled that slot with null.

`constexpr` requires the table's values to be available as constant expressions, allowing initialization without startup code or heap allocation. `const` prevents changes through that declaration, but does not by itself mean initialization happens at compile time. This distinction is why simply adding `const` to the old pointers would not have repaired their initialization order.

The `Chord` record has no custom constructor or destructor. Its members already know how to initialize themselves. This is often called the **rule of zero**: write resource-management functions only when the class actually needs to manage a resource itself.

## 2. Parse before publishing

The parser accepts a `std::string_view`: a temporary, non-owning view of text. It can examine substrings without copying them into ten-byte buffers. It returns a song containing numeric root/chord identifiers and an error message if validation fails.

A string view does not keep its source alive. The parser therefore never stores those views in the resulting song. It stores small numeric records instead. The `std::vector` inside `Song` owns those records and releases their memory automatically when replaced or destroyed.

The application loads and validates into an inactive song slot. It only changes playback after success. This is a transaction-like pattern: either the entire new song becomes active, or the old song remains intact. A file error can no longer accidentally erase the current song.

Two slots make ownership straightforward. The callback reads one immutable song while foreground code prepares the other. Only switching the active playback reference requires a short interrupt-protected operation. File reads, allocations, and destruction occur outside it.

## 3. Give every note an explicit lifetime

`MIDI::voiceChord` computes the final four pitches before anything is transmitted. `VoiceTracker` then produces note-offs for the previously sent voicing and note-ons for the new one. Callers no longer provide arbitrary indices into an internal array.

Packet creation and committing the new tracker state are separate operations. The hardware adapter commits only when the UART accepts the transfer. If transmission later fails, some bytes may already have reached the synth, so it sends MIDI All Sound Off on channel 1 before rebuilding the latest voicing.

The UART sends asynchronously using DMA. Its byte buffer must remain valid and unchanged until completion; a local stack buffer would be unsafe. The buffer is in the MCU's DMA-accessible memory section. An atomic status flag passes the completion result to foreground code; the interrupt never allocates or waits for a lock.

When a second chord arrives during transmission, the adapter finishes the first packet before sending the latest state. It never drops note-off bytes from a packet. This bounds memory use when an external clock is faster than the MIDI wire can handle.

## 4. Choose pitches in absolute space

Separating an input into an octave and pitch class is useful for display, but the nearest note might be in another octave. The quantizer now compares absolute semitone positions in neighboring octaves, excluding candidates outside the hardware's output range.

For example, C-only input at 0.99 V now selects C at 1 V. The result is a named `Note` record containing `voltage`, `pitchClass`, and `octave`. Those names convey meaning directly, and the display no longer guesses the output octave from the input voltage.

The enabled pitches are represented by a twelve-bit mask: bit 0 means C, bit 1 means C-sharp, and so on. Both the keyboard display and quantizer consume this same mask. That prevents two implementations of the jazz threshold from disagreeing.

## 5. Give time-sensitive work a predictable schedule

At 48 kHz, an audio block of 24 samples lasts half a millisecond. Each callback samples the controls and gates, advances or resets playback, calculates CV, applies oscillator pitch changes, and generates those 24 audio samples.

The foreground loop handles work whose duration varies: drawing the display, loading songs, and starting MIDI transfers. A small snapshot passes playback state to the display and MIDI adapter. It contains values only—no owning strings or vectors—so copying it is bounded and does not allocate memory.

`ScopedIrqBlocker` temporarily disables interrupts while foreground code copies this snapshot or updates a shared reference. Its destructor restores the prior interrupt state when the scope ends, including on an early return. This automatic cleanup is **RAII**. It is useful even when exceptions are disabled.

`volatile` would not be a substitute: it cannot make a group of related fields consistent. Interrupt exclusion protects the small compound operation; the DMA status uses an atomic because it is a single independently published value.

Oscillators are initialized before audio starts, and only the callback updates them during playback. There is one audio-start call.

## 6. Boundaries and errors are part of the design

No-song is now a valid state with silent/zero outputs. Root/type errors name the chord position. Files over 4095 bytes are rejected explicitly. Read and close failures preserve the previous song. Output DAC codes saturate at 4095, including when quantization rounds up to the top endpoint.

The parser's main contract is: a successful song contains only valid events. Playback's contract is: its song remains alive and unchanged while it is active. MIDI's contract is: every sent note is tracked until its note-off. These contracts are more useful than scattering defensive checks through every caller.

## 7. Tests protect behavior during cleanup

The desktop tests compile the actual musical code. They check known bug examples and general properties: increasing jazz never removes notes, increasing voltage never lowers a quantized result within one scale, no enabled in-range note is closer than the selected note, and MIDI transitions leave no orphaned notes.

A second test executable compiles the actual application with hardware substitutes. It checks failed loads, repeated song swaps, reset, screen text, and asynchronous MIDI behavior. Every header is also compiled independently, catching accidental dependence on another header's includes.

AddressSanitizer and UndefinedBehaviorSanitizer detect many invalid memory accesses and arithmetic errors. They cannot verify electrical voltages, interrupt timing, SD-card compatibility, or how a particular synthesizer responds. The ARM build and the hardware checklist cover different parts of confidence in the result.
