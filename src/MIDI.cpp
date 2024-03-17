#include "daisysp.h"
#include "daisy_patch.h"

using namespace daisy;
using namespace daisysp;

extern DaisyPatch patch;

namespace MIDI {
    int activeNotes [4];

    // --- Functions --- //

    void MIDISendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, int index) 
    {
    activeNotes[index] = note;
    uint8_t data[3] = { 0 };
    
    data[0] = (channel & 0x0F) + 0x90;  // limit channel byte, add status byte
    data[1] = note & 0x7F;              // remove MSB on data
    data[2] = velocity & 0x7F;

    patch.midi.SendMessage(data, 3);
    };

    void MIDISendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) 
    {
        uint8_t data[3] = { 0 };

        data[0] = (channel & 0x0F) + 0x80;  // limit channel byte, add status byte
        data[1] = note & 0x7F;              // remove MSB on data
        data[2] = velocity & 0x7F;

        patch.midi.SendMessage(data, 3);
    }

    void clearMidi() {
        for (int i = 0; i < 4; i++){
            MIDISendNoteOff(0, activeNotes[i], 100);
        }
    }
}