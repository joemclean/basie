#include "daisysp.h"
#include "daisy_patch.h"
#include "src/theory.hpp"
#include "src/quantizer.hpp"
#include "src/sdhandler.hpp"

#include "fatfs.h"

// for convenience

using std::array;
using std::string;
using std::vector;
using std::pair;

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

// ---------My variables-------- //

static DaisyPatch hw;

vector<string> fileList;
int fileListCursor = 0;
int loadedFileIndex = 0;
int fileListPageIndex = 0;

int displayTabIndex = 0;
bool encoderIsHeld = false;
bool tabChangeInProcess = false;

bool beatChanging = false;

int activeNotes [4];

array<float, 12> targetScale;
float jazzAmount = 0.5;

string errorMessage = "Test message";

int selectedSongIndex = 0;
string chordDisplay = "";
string songName = "-";

vector<string> currentSongChords;

string displayLineOne = "";
string displayLineTwo = "";
string displayLineThree = "";

int playhead = 0;

int chordType = 0;
int chordRootIndex = 0;


// ----------------- //

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
void UpdateOutputs();
void Process();

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

void loadSong(string fileName) 
{
    clearMidi();
    string fileChords = loadChordsFromFile(fileName);
    currentSongChords = parseChords(fileChords);
    displayLineOne = fileName;
    playhead = 0;
}

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

void Process()
{                                                                                                                   
    // Advance the playhead on clock trigger
    Chord *targetChord;
    
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
    string chord = currentSongChords[playhead];

    string chordRoot = "C";
    string chordType = "maj7";

    char chordBuffer[10];
    strcpy(chordBuffer, chord.c_str());

    char chordRootBuffer[10];
    char chordTypeBuffer[10];

    strcpy(chordRootBuffer, strtok(chordBuffer, " "));
    strcpy(chordTypeBuffer, strtok(NULL, " "));
    // Prepare the chord and scale based on the chord type

    chordRoot = chordRootBuffer;
    chordType = chordTypeBuffer;

    // Migration TODO: Unused
    // float chordVoltages[7];

    for (int i = 0; i < 12; i++) {
        if (chordRoot == noteDisplayNames[i])
        {
            chordRootIndex = i;
        }
    }

    targetChord = chordList[0]; // initialize with default
    for (size_t i = 0; i < chordList.size(); i++) {
        if (chordList[i]->displayName == chordType) {
            targetChord = chordList[i];
        }
    }

    // Iterate over chord tones to get an array of voltages
    float chordRootOffsetVoltage = (float)chordRootIndex / 12.0;
    int chordToneCount = targetChord->tones.size();

    for (int i = 0; i < 12; i++) 
    {
        // Migration TODO: UPDATE INTERFACE
        // setRGBBrightness(chordLights[i], 0, 0, 0);
        // setRGBBrightness(quantizerLights[i], 0, 0, 0);
        // setRGBBrightness(quantizer2Lights[i], 0, 0, 0);
    }
    if (beatChanging) {
        clearMidi();
    }

    for (int i = 0; i < chordToneCount; i++)
    {   
        int chordTone = targetChord->tones[i];
        // TODO Abstract this
        // Migration TODO: Reimplement voicing type
        // Migration TODO: UI
        int targetMidiNote = 36 + chordRootIndex + chordTone;
        if (beatChanging == true && i < 4)
        {
            if (i > 0) {
                targetMidiNote = targetMidiNote + 12;
            }
            activeNotes[i] = targetMidiNote;
            MIDISendNoteOn(0, targetMidiNote, 100);
        }

        // int targetLightIndex = (chordRootIndex + chordTone) % 12;
        // int targetLightIndex = (chordRootIndex + chordTone) % 12;
        // setRGBBrightness(chordLights[targetLightIndex], 0.1, 0.1, 0.05);
    }

    // Write the chord tones to output
    // Migration TODO: Write output values

    // outputs[CV_OUTPUT_1].setVoltage(chordVoltages[0]);
    // outputs[CV_OUTPUT_2].setVoltage(chordVoltages[1]);
    // outputs[CV_OUTPUT_3].setVoltage(chordVoltages[2]);
    // outputs[CV_OUTPUT_4].setVoltage(chordVoltages[3]);
    // outputs[CV_OUTPUT_5].setVoltage(chordVoltages[4]);
    // outputs[CV_OUTPUT_6].setVoltage(chordVoltages[5]);
    // outputs[CV_OUTPUT_7].setVoltage(chordVoltages[6]);

    // Update the display with human names for the current chord
    chordDisplay = noteDisplayNames[chordRootIndex] + targetChord->displayName;

    // Migration TODO: Update display
    displayLineTwo = "Current chord:";
    displayLineThree = chordDisplay; 

    // Quantize input to output
    targetScale = targetChord->chordScale;

    // Migration TODO: Actually read Jazz param
    float jazzKnob =  0.5;
    // float jazzKnob =  params[JAZZ_PARAM].getValue();
    float jazzCV = 0.5;
    // float jazzCV = params[JAZZ_AMT_IN].getValue();
    float jazzCVAttenuation = 0.5;
    // float jazzCVAttenuation = params[JAZZ_CV_ATTENUATOR].getValue();
    // float jazzAmount = jazzKnob + ((jazzCV / 10.0) * jazzCVAttenuation);

    if (jazzAmount > 1) jazzAmount = 1;
    if (jazzAmount < 0) jazzAmount = 0;

    // Read quantizer ins
    float voice1Voltage = patch.GetKnobValue((DaisyPatch::Ctrl)0) * 1.f;
    float voice2Voltage = patch.GetKnobValue((DaisyPatch::Ctrl)1) * 1.f;

    pair<float, int> values1 = quantizeToScale(voice1Voltage, chordRootOffsetVoltage, targetScale, jazzAmount); //jazzAmt used to be last arg
    pair<float, int> values2 = quantizeToScale(voice2Voltage, chordRootOffsetVoltage, targetScale, jazzAmount); //jazzAmt used to be last arg

    float note1QuantizedVoltage = values1.first;
    float note2QuantizedVoltage = values2.first;

    // Migration TODO: Set quantizer UI
    // setRGBBrightness(quantizerLights[(index1 + chordRootIndex) % 12], 0.0, 0.0, 1.0);
    // setRGBBrightness(quantizer2Lights[(index2 + chordRootIndex) % 12], 0.0, 1.0, 0);

    patch.seed.dac.WriteValue(DacHandle::Channel::ONE,
                              note1QuantizedVoltage * 819.2f);
    patch.seed.dac.WriteValue(DacHandle::Channel::TWO,
                              note2QuantizedVoltage * 819.2f);

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


    int targetScaleSize = targetScale.size();

    bool shouldFill;
    int whiteKeyIndex = 0;
    int blackKeyIndex = 0;

    int keyHeight = 14;
    int keyWidth = 6;
    int blackKeyOffset = 3;
    int keyGap = 2;
    int startingY = 32;

    for (int i = 0; i < targetScaleSize; i++) {
        // Migration TODO: is this still used?
        int targetIndex = (i + 12 - chordRootIndex) % 12;
        // TODO weirdly inverted?
        shouldFill = false;

        if (targetScale[targetIndex] >= (1 - jazzAmount))
        {
            shouldFill = true;
        }
        if (i == 0 || i == 2 || i == 4 || i == 5 || i == 7 || i == 9 || i == 11) {
            patch.display.DrawRect(
                whiteKeyIndex * (keyWidth + keyGap), 
                startingY + keyHeight + keyGap, 
                (whiteKeyIndex + 1) * (keyWidth + keyGap), 
                startingY + (2 * keyHeight) + keyGap, 
                true, 
                shouldFill
            );
            whiteKeyIndex++;
        } else {
            patch.display.DrawRect(
                blackKeyIndex * (keyWidth + keyGap) + blackKeyOffset, 
                startingY, 
                (blackKeyIndex + 1) * (keyWidth + keyGap) + blackKeyOffset, 
                startingY + keyHeight, 
                true, 
                shouldFill
            );
            blackKeyIndex++;
        }
        if (i == 4) {
            blackKeyIndex++;
        }
    }

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
}


