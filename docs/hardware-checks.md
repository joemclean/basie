# Daisy Patch verification

Desktop tests and an ARM build cannot establish electrical or real-time behavior. Run these checks on the Patch before relying on the firmware in a performance.

- Boot without a card, with an empty card, with a malformed first file, and with a valid card. Invalid/no-song startup should leave CV and audio at zero and send no MIDI note-ons. Errors must be readable and dismissible.
- Load the sample progression. Check the first chord before any clock. Advance, wrap, reset mid-song, and present clock/reset together. CV, oscillator pitch, displayed chord, and the next MIDI voicing must agree. Reset takes priority when both edges are sampled together.
- Capture MIDI on channel 1. C major seventh should send notes 36, 52, 55, and 59 once each. Every new voicing must send four matching note-offs before four note-ons. Repeated identical chords should retrigger without accumulating voices.
- Compare CV against a calibrated meter/tuner. Sweep both pitch inputs across every octave boundary and the top endpoint. With only C enabled, 0.99 V should select approximately 1 V. Confirm no wrap to 0 V near 5 V. Measure offset/gain error before altering the nominal conversion factor.
- Check C min7 at jazz 0.5: B-flat must remain B-flat rather than becoming B natural. Verify the keyboard mask and selected octave agree with the measured output.
- Use 1 ms clock/reset pulses with at least 1 ms low time between pulses. Sweep frequency while turning the encoder, updating the display, and loading a song. Use a logic analyzer or oscilloscope to measure missed edges, maximum callback duration, and clock-to-CV latency. Pulses below the 0.5 ms sampling period are not guaranteed.
- Stress MIDI faster than its approximately 7.7 ms four-off/four-on wire time. Intermediate MIDI voicings may be skipped, but the final voicing must be correct and no old notes should hang. CV/audio should still follow sampled edges. SD I/O may defer starting the next MIDI packet.
- Load an invalid or oversized file while another song plays. The current song must continue, with an error explaining why the load failed. Check that directory/file failures do not hang the application; library SD timeouts may still take time to return.
- Check all three sine outputs and their mix. Listen for discontinuities or glitches while changing chords/loading songs, and verify that boot without a song is silent. The existing small oscillator detuning is intentional.
- Repeat with long filenames, multiple file-browser pages, upper-case `.TXT` extensions, and repeated song changes. Check flash/RAM usage and observe stability over a long run.

The initial implementation supports the standard internal-flash build. Moving the application to another bootloader/memory layout requires rechecking FatFs and DMA buffer placement.
