# Basie

## Author
Joe McLean

## Description
Basie is a module designed to run on the Daisy Patch. It provides a 2-channel quantizer that follows a pre-set chord progression, provided via text files that can be provided via the SD card.

[Source Code](https://github.com/electro-smith/DaisyExamples/tree/master/patch/Sequencer)

## Controls

| Control | Description | Comment |
| --- | --- | --- |
| Gate In 1 | Advance Sequence | Advance the chord progression forward one beat, based on the clock input |
| Gate In 2 | Reset sequence | Reset the sequence to the first beat |
| CV 1 | Quantizer 1 in | Provide a CV signal to quantize |
| CV 2 | Quantizer 2 in | Provide a CV signal to quantize |
| CV 3 | Jazz control 1 | Controls the number of candidate notes, in increasing order of dissonance |
| CV 4 | Jazz control 2 | PControls the number of candidate notes, in increasing order of dissonance |
| CV 1 Out | Quantizer 1 out | Quantized 1v/oct note |
| CV 2 Out | Quantizer 2 out | Quantized 1v/oct note |

| Encoder (Press and turn) | Switch between tabs | Press and hold to switch between the song list and song view |
| Encoder (turn) | Scroll file list | Move up and down the file list in the song list view |
| Encoder (press) | Select song | Select the currently highlighted song in the song view |


