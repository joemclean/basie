#ifndef MIDI_H
#define MIDI_H

void MIDISendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, int index);

void MIDISendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);

void clearMidi();

#endif