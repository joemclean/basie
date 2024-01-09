# Basie

## Author
Joe McLean

## Description
A Eurorack module built on the Daisy platform, designed to create generative modular patches over jazz chord progressions. Load chord sequences from an SD card. Use a clock signal to advance through chords, output as MIDI notes. A two-channel quantizer follows the chord progression, "snapping" input notes to harmonically compatible notes at a strength you choose. Quanitzation strength is also CV controllable.

## Controls

| Control | Description | Comment |
| --- | --- | --- |
| **Gate In 1** | Advance Sequence | Recieves a clock pulse and advances the sequence by 1 step |
| **Gate In 2** | Reset | Resets sequence to the beginning |
| **CV 1** | Quantizer 1 in | Input a note signal to be quantized to the current chord. |
| **CV 2** | Quantizer 2 in | Input a second note signal to be quantized to the current chord. |
| **CV 3** | Quantizer 1 Jazz amount | Control quantizer strength. Low values = root note only, mid values = chord tones, high values = chromatic. CV controllable. |
| **CV 4** | Quantizer 2 Jazz amount | Same but for quantizer 2 |
| **MIDI out** | Chord MIDI out | Outputs 1st, 3rd, 5th, and 7th up the current chord as MIDI notes on each beat. |
| **Hold + turn encoder** | Switch tabs | Switch between the file list and the sequence display |
| **Turn encoder** (file list only) | Navigate files | Scroll up and down in file list |
| **Press encoder** (file list only) | Load file | Load the current file indicated by the cursor |
