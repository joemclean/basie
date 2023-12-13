#include "daisysp.h"
#include "daisy_patch.h"

#include "src/theory.hpp"
#include "src/quantizer.hpp"
#include "src/sdhandler.hpp"
#include "src/display.hpp"

#include "fatfs.h"

// for convenience

using std::array;
using std::string;
using std::vector;
using std::pair;

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;
static DaisyPatch hw;

// --- System variables --- //

// Display variables
vector<string> fileList;
int fileListCursor = 0;
int loadedFileIndex = 0;
int fileListPageIndex = 0;

string displayLineOne = "";
string displayLineTwo = "";
string displayLineThree = "";
string chordDisplay;

int displayTabIndex = 0;
bool encoderIsHeld = false;
bool tabChangeInProcess = false;

// Module state variables
vector<string> currentSongChords;
int playhead = 0;

bool beatChanging = false;

int chordType = 0;
int chordRootIndex = 0;

Chord *targetChord;
array<float, 12> targetScale;

int activeNotes [4];

float jazzAmountCh1 = 0.5;
float jazzAmountCh2 = 0.5;

float note1QuantizedVoltage = 0.0;
float note2QuantizedVoltage = 0.0;

// --- Functions --- //

void MIDISendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    uint8_t data[3] = { 0 };
    
    data[0] = (channel & 0x0F) + 0x90;  // limit channel byte, add status byte
    data[1] = note & 0x7F;              // remove MSB on data
    data[2] = velocity & 0x7F;

    patch.midi.SendMessage(data, 3);
};

void MIDISendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
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

void UpdateControls();
void UpdateOled();
void Process();
void UpdateOutputs();

// Helper function to trim whitespace from the start and end of a string
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \n\r\t\f\v");
    if (first == string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \n\r\t\f\v");
    return str.substr(first, (last - first + 1));
}

// Parse chords string into a vector that can be iterated over
vector<string> parseChords(const string& chordString) {
    vector<string> chords;
    string chord;
    bool insideChord = false;

    for (char c : chordString) {
        if (c == '|') {
            if (!chord.empty()) {
                chords.push_back(trim(chord));
                chord.clear();
            }
            insideChord = false;
        } else if (!isspace(static_cast<unsigned char>(c)) || insideChord) {
            insideChord = true;
            chord += c;
        }
    }

    // Add the last chord if there is one
    if (!chord.empty()) {
        chords.push_back(trim(chord));
    }

    return chords;
}

void loadSong(const string& fileName) 
{
    clearMidi();
    string fileChords = loadChordsFromFile(fileName);
    currentSongChords = parseChords(fileChords);
    displayLineOne = fileName;
    playhead = 0;
}

void ProcessEncoder()
{
    if (patch.encoder.RisingEdge()) {
        encoderIsHeld = true;
    }
    if (patch.encoder.FallingEdge()) {
        encoderIsHeld = false;
        if (!tabChangeInProcess && displayTabIndex == 1) {
            loadSong(fileList[fileListCursor]);
            loadedFileIndex = fileListCursor;
        }
        tabChangeInProcess = false;
    }
    int increment = patch.encoder.Increment();
    if (increment != 0)
    {
        if (encoderIsHeld) {
            // Change tab
            displayTabIndex++;
            displayTabIndex = displayTabIndex % 2;
            tabChangeInProcess = true;
        } else {
            // Navigate file system
            int fileListSize = static_cast<int>(fileList.size());
            fileListCursor += increment;

            // Wrap around logic
            if (fileListCursor >= fileListSize) {
                fileListCursor = 0;
            } else if (fileListCursor < 0) {
                fileListCursor = fileListSize > 0 ? fileListSize - 1 : 0;
            }
        }
    }
    UpdateOled();
}

void UpdateControls()
{
    patch.ProcessAnalogControls();
    patch.ProcessDigitalControls();
    ProcessEncoder();
}

// --- Implementation --- //

int main(void)
{
    patch.Init(); // Initialize hardware (daisy seed, and patch)

    fileList = listTxtFiles("/");
    loadSong(fileList[0]);

    patch.StartAdc();
    while(1)
    {
        UpdateControls();
        UpdateOled();
        Process();
        UpdateOutputs();
    }

    for (size_t i = 0; i < chordList.size(); i++) {
        delete chordList[i];
    }
}

void Process()
{                                                                                                                   
    // Advance the playhead on clock trigger
    if (patch.gate_input[0].Trig())
    {
        beatChanging = true;
        int songLength = currentSongChords.size();
        playhead = (playhead + 1) % songLength;
    }

    // Reset the playhead on reset trigger
    if (patch.gate_input[1].Trig())
    {
        playhead = 0;
    }

    // TODO - only do all of this if the playhead advances
    string chordString = currentSongChords[playhead];

    string chordRoot = "C";
    string chordType = "maj7";

    char chordBuffer[10];
    strcpy(chordBuffer, chordString.c_str());

    char chordRootBuffer[10];
    char chordTypeBuffer[10];

    strcpy(chordRootBuffer, strtok(chordBuffer, " "));
    strcpy(chordTypeBuffer, strtok(NULL, " "));

    // Prepare the chord and scale based on the chord type
    chordRoot = chordRootBuffer;
    chordType = chordTypeBuffer;

    for (int i = 0; i < 12; i++) {
        if (chordRoot == noteDisplayNames[i])
        {
            chordRootIndex = i;
        }
    }

    // Match the current progression chord against available chords
    targetChord = chordList[0]; // initialize with default
    for (size_t i = 0; i < chordList.size(); i++) {
        if (chordList[i]->displayName == chordType) {
            targetChord = chordList[i];
        }
    }

    // Iterate over active chord tones and output as MIDI notes
    int chordToneCount = targetChord->tones.size();
    if (beatChanging) {
        clearMidi();
        for (int i = 0; i < chordToneCount; i++)
        {   
            int chordTone = targetChord->tones[i];  
            // Migration TODO: Reimplement voicing type
            int targetMidiNote = 36 + chordRootIndex + chordTone;
            if (i < 4)
            {
                if (i > 0) {
                    targetMidiNote = targetMidiNote + 12;
                }
                activeNotes[i] = targetMidiNote;
                MIDISendNoteOn(0, targetMidiNote, 100);
            }
        }
    }

    // Update the display with human names for the current chord
    chordDisplay = noteDisplayNames[chordRootIndex] + targetChord->displayName;

    displayLineTwo = "Chord " + std::to_string(playhead + 1) + "/" + std::to_string(currentSongChords.size()) + ":";
    displayLineThree = chordDisplay; 

    // Quantize input to output
    targetScale = targetChord->chordScale;

    jazzAmountCh1 = patch.GetKnobValue((DaisyPatch::Ctrl)2) * 1.f;
    jazzAmountCh2 = patch.GetKnobValue((DaisyPatch::Ctrl)3) * 1.f;

    if (jazzAmountCh1 > 1) jazzAmountCh1 = 1;
    if (jazzAmountCh1 < 0) jazzAmountCh1 = 0;


    if (jazzAmountCh2 > 1) jazzAmountCh2 = 1;
    if (jazzAmountCh2 < 0) jazzAmountCh2 = 0;

    // Read quantizer ins
    float voice1Voltage = patch.GetKnobValue((DaisyPatch::Ctrl)0) * 5.f;
    float voice2Voltage = patch.GetKnobValue((DaisyPatch::Ctrl)1) * 5.f;

    // Quantize the inputs to active notes in the target scale
    float chordRootOffsetVoltage = (float)chordRootIndex / 12.0;
    pair<float, int> values1 = quantizeToScale(voice1Voltage, chordRootOffsetVoltage, targetScale, jazzAmountCh1);
    pair<float, int> values2 = quantizeToScale(voice2Voltage, chordRootOffsetVoltage, targetScale, jazzAmountCh2);

    note1QuantizedVoltage = values1.first;
    note2QuantizedVoltage = values2.first;

    beatChanging = false;
}

void UpdateOled()
{
    patch.display.Fill(false);
    if (displayTabIndex == 0) 
    {
        string str  = displayLineOne;
        char*       cstr = &str[0];

        patch.display.SetCursor(0, 0);
        patch.display.WriteString(cstr, Font_7x10, true);

        patch.display.SetCursor(0, 10);
        str = displayLineTwo;
        patch.display.WriteString(cstr, Font_7x10, true);

        patch.display.SetCursor(0, 20);
        str = displayLineThree;
        patch.display.WriteString(cstr, Font_7x10, true);

        patch.display.SetCursor(0, 30);

        size_t targetScaleSize = targetScale.size();

        int ch2XOffset = 50;

        drawKeyboard(targetScale, targetScaleSize, jazzAmountCh1, chordRootIndex, 0, 30);
        drawKeyboard(targetScale, targetScaleSize, jazzAmountCh2, chordRootIndex, ch2XOffset, 30);
    } else if (displayTabIndex == 1) {
        string headerStr = "File browser";
        patch.display.SetCursor(0, 0);
        patch.display.WriteString(headerStr.c_str(), Font_7x10, true);

        int filesPerPage = 5;
        fileListPageIndex = (int)fileListCursor / filesPerPage ;
        int lowerPageBound = fileListPageIndex * filesPerPage ;
        int fileListLength = static_cast<int>(fileList.size());

        int upperPageBound = (fileListPageIndex + 1) * filesPerPage;
        if (upperPageBound > fileListLength) {
            upperPageBound = fileListLength;
        }

        for (int i = lowerPageBound; i < upperPageBound; i++ ) {
            patch.display.SetCursor(0, (i + 1 - (filesPerPage * fileListPageIndex)) * 10);
            string fileStr;
            fileListCursor == i ? fileStr += ">" : fileStr += " ";
            loadedFileIndex == i ? fileStr += "*" : fileStr += " ";
            fileStr += fileList[i];
            patch.display.WriteString(fileStr.c_str(), Font_7x10, true);
        }
    }

    patch.display.Update();
}

void UpdateOutputs()
{
    patch.seed.dac.WriteValue(DacHandle::Channel::ONE,
                              note1QuantizedVoltage * 819.2f);
    patch.seed.dac.WriteValue(DacHandle::Channel::TWO,
                              note2QuantizedVoltage * 819.2f);
}
