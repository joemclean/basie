#include "daisysp.h"
#include "daisy_patch.h"
#include <string>
#include <array>

#include "fatfs.h"

// for convenience

using std::array;
using std::string;

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

// ---------My variables-------- //

static DaisyPatch hw;

SdmmcHandler   sd;
FatFSInterface fsi;


DIR dir;
FILINFO fno;

FIL file;       // File object
FRESULT fr;     // FatFs return code
UINT br;        // Read count

std::vector<std::string> fileList;
int fileListCursor = 0;
int loadedFileIndex = 0;

int displayTabIndex = 1;
bool encoderIsHeld = false;

string errorMessage = "Test message";

constexpr size_t bufferSize = 4096; // Example size, adjust as needed
char buffer[bufferSize] = {0};

int selectedSongIndex = 0;
string chordDisplay = "";
string songName = "-";

std::vector<std::string> currentSongChords;

string displayLineOne = "";
string displayLineTwo = "";
string displayLineThree = "";

int playhead = 0;

int chordType = 0;


// ----------------- //


std::vector<std::string> listTxtFiles(const char* path) {

    std::vector<std::string> txtFiles;

    // Init SD Card
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sd.Init(sd_cfg);

    // Links libdaisy i/o to fatfs driver.
    fsi.Init(FatFSInterface::Config::MEDIA_SD);

    // Mount SD Card
    f_mount(&fsi.GetSDFileSystem(), "/", 1);

    fr = f_opendir(&dir, path);  // Open the directory
    if (fr == FR_OK) {
        for (;;) {
            fr = f_readdir(&dir, &fno);  // Read a directory item
            if (fr != FR_OK || fno.fname[0] == 0) break;  // Break on error or end of dir
            if (fno.fattrib & AM_DIR) {
                // It's a directory, you can ignore or handle accordingly
            } else { 
                // It's a file, check if it's a .txt file
                std::string fileName = fno.fname;
                if (fileName.size() >= 4 && fileName.substr(fileName.size() - 4) == ".txt") {
                    txtFiles.push_back(fileName);
                    errorMessage = fileName;
                }
            }
        }
        f_closedir(&dir);
    } else {
        errorMessage = "Failed to load directory"; 
    }

    return txtFiles;
}

std::string loadChordsFromFile(void)
{
    displayLineOne = "Starting load";
    std::string chordData;

    // Init SD Card
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sd.Init(sd_cfg);

    // Links libdaisy i/o to fatfs driver.
    fsi.Init(FatFSInterface::Config::MEDIA_SD);

    // Mount SD Card
    f_mount(&fsi.GetSDFileSystem(), "/", 1);

    // Try to open the file
    fr = f_open(&file, "autumnLeaves.txt", FA_READ);

    if (fr == FR_OK) {
        // Read the entire file into the buffer
        fr = f_read(&file, buffer, bufferSize - 1, &br);
        if (fr == FR_OK) {
            // Ensure there's a null terminator at the end of the read data
            buffer[br] = '\0';
            
            // Now 'buffer' contains the entire file content as a C-style string
            std::string fileContent(buffer);
            displayLineOne = fileContent;
            chordData = fileContent;

        } else {
            displayLineOne = "Read error: " + std::to_string(fr);
        }
        // Close the file
        f_close(&file);
    } else {
        displayLineOne = "File open error";
    }

    return chordData;
}

void UpdateControls();
void UpdateOled();
void UpdateOutputs();
void Process();

// ----------------- //
// Theory setup
// ----------------- //

// Define chords as half steps above root
struct Chord
{
    array<int, 7> tones;
    string displayName;
    array<float, 12> chordScale;
    Chord(array<int, 7> tones, string displayName, array<float, 12> chordScale)
    {
        this->tones = tones;
        this->displayName = displayName;
        this->chordScale = chordScale;
    }
    ~Chord()
    {
    }
};

array<float, 12> ionianScale = {1, 0, 0.1, 0, 0.5, 0.3, 0, 0.7, 0, 0.3, 0, 0.5};
array<float, 12> dorianScale = {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0};
array<float, 12> phrygianScale = {1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0};
array<float, 12> lydianScale = {1, 0, 0.1, 0, 0.5, 0, 0.7, 0.7, 0, 0.3, 0, 0.6};
array<float, 12> lydianDominantScale = {1, 0, 0.1, 0, 0.5, 0, 0.7, 0.7, 0, 0.3, 0.6, 0};
array<float, 12> mixolydianScale = {1, 0, 0.1, 0, 0.5, 0.3, 0, 0.7, 0, 0.3, 0.5, 0};
array<float, 12> mixolydianFlat2Scale = {1, 0.7, 0, 0, 0.5, 0.2, 0, 0.7, 0, 0.2, 0.6, 0};
array<float, 12> mixolydianFlat13Scale = {1, 0, 0.1, 0, 0.5, 1, 0, 0.8, 0.7, 0, 0.6, 0};
array<float, 12> aeoleanScale = {1, 0, 0.1, 0.5, 0, 0.3, 0, 0.7, 0, 0.3, 0, 0.5};
array<float, 12> locrianScale = {1, 0.1, 0, 0.5, 0, 0.3, 0.7, 0, 0.3, 0, 0.5, 0};
array<float, 12> diminishedScale = {1, 0, 0.6, 0.8, 0, 0.6, 0.8, 0, 0.6, 0.8, 0, 0.6};
array<float, 12> alteredScale = {1, 0.3, 0, 0.5, 0.1, 0, 0.8, 0, 0.4, 0, 0.6, 0};
array<float, 12> augmentedScale = {1, 0, 0.2, 0, 0.5, 0, 0, 0.2, 0.8, 0, 0, 0.2};

// TODO not actually sure 13 is relevant for lots of these
// TODO more than one scale per chord
Chord *majorChord = new Chord({0, 4, 7, 11, 14, 17, 21}, "maj7", ionianScale);
Chord *minorChord = new Chord({0, 3, 7, 10, 14, 17, 21}, "min7", aeoleanScale);
Chord *major6Chord = new Chord({0, 4, 7, 9, 14, 17, 21}, "6", ionianScale);
Chord *minor6Chord = new Chord({0, 3, 7, 9, 14, 17, 21}, "min6", dorianScale);
Chord *dominantChord = new Chord({0, 4, 7, 10, 14, 17, 21}, "7", mixolydianScale);
Chord *halfDiminishedChord = new Chord({0, 3, 6, 10, 14, 17, 21}, "halfdim7", locrianScale);
Chord *diminishedChord = new Chord({0, 3, 6, 9, 12, 15, 21}, "dim7", diminishedScale);
Chord *flat9chord = new Chord({0, 4, 7, 10, 13, 17, 21}, "7b9", mixolydianFlat2Scale);
Chord *sharp9chord = new Chord({0, 4, 7, 10, 15, 17, 21}, "7#9", alteredScale);
Chord *sharp11chord = new Chord({0, 4, 7, 11, 14, 18, 21}, "maj7#11", lydianScale);
Chord *dominantSharp11chord = new Chord({0, 4, 7, 10, 14, 18, 21}, "7#11", lydianDominantScale);
Chord *flat13chord = new Chord({0, 4, 7, 10, 14, 17, 20}, "7b13", mixolydianFlat13Scale);
Chord *augmentedChord = new Chord({0, 4, 8, 10, 14, 17, 21}, "aug", augmentedScale);

// TODO OOPS DOESN'T WORK W SHARPS :(
array<string, 12> noteDisplayNames = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};

array<Chord *, 14> chordList = {majorChord, minorChord, major6Chord, minor6Chord, dominantChord, halfDiminishedChord, diminishedChord, flat9chord, sharp9chord, sharp11chord, dominantSharp11chord, flat13chord, augmentedChord };

struct ChordScale
{
    std::array<int, 12> tones;
};

std::pair<float, int> quantizeToScale(const float& noteInVoltage, const float& chordRootOffsetVoltage, const array<float, 12>& targetScale, const float& jazzAmount)
{
    float whole, fractional;
    float jazzThreshold = 1.0 - jazzAmount;
    whole = floor(noteInVoltage);
    fractional = noteInVoltage - whole;
    int nearestScaleToneIndex;
    float currentLowestNoteDistance = 1.0;
    for (int i = 0; i < 12; i++) {
        if ((float)targetScale[i] >= jazzThreshold) {
            float indexAsFloat = (float)i;
            float indexAsVoltage = (indexAsFloat/12) + chordRootOffsetVoltage;
            if (indexAsVoltage > 1.0) {
                indexAsVoltage = indexAsVoltage - 1;
            }
            float noteDistance = abs(indexAsVoltage - fractional);
            if (noteDistance < currentLowestNoteDistance) {
                currentLowestNoteDistance = noteDistance;
                nearestScaleToneIndex = i;
            }
        }
    }
    float baseNoteVoltage = (float)nearestScaleToneIndex / 12;
    float targetNoteVoltage = whole + baseNoteVoltage + chordRootOffsetVoltage;
    return std::make_pair(targetNoteVoltage, nearestScaleToneIndex);
}

// Helper function to trim whitespace from the start and end of a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \n\r\t\f\v");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \n\r\t\f\v");
    return str.substr(first, (last - first + 1));
}

// Parse chords string into a vector that can be iterated over
std::vector<std::string> parseChords(const std::string& chordString) {
    std::vector<std::string> chords;
    std::string chord;
    bool insideChord = false;

    for (char c : chordString) {
        if (c == '|') {
            if (!chord.empty()) {
                chords.push_back(trim(chord));
                chord.clear();
            }
            insideChord = false;
        } else if (!std::isspace(static_cast<unsigned char>(c)) || insideChord) {
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

int main(void)
{
    patch.Init(); // Initialize hardware (daisy seed, and patch)

    fileList = listTxtFiles("/");
    string fileChords = loadChordsFromFile();

   currentSongChords = parseChords(fileChords);

    patch.StartAdc();
    while(1)
    {
        UpdateControls();
        UpdateOled();
        Process();
        UpdateOutputs();
    }

    for (std::size_t i = 0; i < chordList.size(); i++) {
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
    }
    int increment = patch.encoder.Increment();
    if (increment != 0)
    {
        if (encoderIsHeld) {
            displayTabIndex++;
            displayTabIndex = displayTabIndex % 2;
        } else {
            // navigate file system
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
        int songLength = currentSongChords.size();
        playhead = (playhead + 1) % songLength;
    }

    // Reset the playhead on reset trigger
    if (patch.gate_input[1].Trig())
    {
        playhead = 0;
    }

    // TODO - only do all of this if the playhead advances
    std::string chord = currentSongChords[playhead];

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
    float chordVoltages[7];
    array<float, 12> targetScale;

    int chordRootIndex = 0;
    for (int i = 0; i < 12; i++) {
        if (chordRoot == noteDisplayNames[i])
        {
            chordRootIndex = i;
        }
    }

    targetChord = majorChord;
    for (std::size_t i = 0; i < chordList.size(); i++) {
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

    for (int i = 0; i < chordToneCount; i++)
    {   
        int chordTone = targetChord->tones[i];
        // TODO Abstract this
        // Migration TODO: Reimplement voicing type
        // Migration TODO: UI
        // int targetLightIndex = (chordRootIndex + chordTone) % 12;
        // setRGBBrightness(chordLights[targetLightIndex], 0.1, 0.1, 0.05);
        if (i < 4)
        {
            // Migration TODO: UI
            // setRGBBrightness(chordLights[targetLightIndex], 0.4, 0.4, 0.2);
        }
        if (i == 0) {
            // Migration TODO: UI
            // setRGBBrightness(chordLights[targetLightIndex], 1.0, 1.0, 1.0);
        }

        float chordToneAsFloat = (float)chordTone;
        float noteVoltage = (chordToneAsFloat / 12.0) + chordRootOffsetVoltage;
        chordVoltages[i] = noteVoltage;
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
    float jazzAmount = jazzKnob + ((jazzCV / 10.0) * jazzCVAttenuation);

    if (jazzAmount > 1) jazzAmount = 1;
    if (jazzAmount < 0) jazzAmount = 0;

    // Read quantizer ins
    float voice1Voltage = patch.GetKnobValue((DaisyPatch::Ctrl)0) * 1.f;
    float voice2Voltage = patch.GetKnobValue((DaisyPatch::Ctrl)1) * 1.f;

    std::pair<float, int> values1 = quantizeToScale(voice1Voltage, chordRootOffsetVoltage, targetScale, 0.5); //jazzAmt used to be last arg
    std::pair<float, int> values2 = quantizeToScale(voice2Voltage, chordRootOffsetVoltage, targetScale, 0.5); //jazzAmt used to be last arg

    float note1QuantizedVoltage = values1.first;
    float note2QuantizedVoltage = values2.first;

    int targetScaleSize = targetScale.size();
    for (int i = 0; i < targetScaleSize; i++) {
        // Migration TODO: is this still used?
        // int targetIndex = (i + chordRootIndex) % 12;
        // TODO weirdly inverted?
        if (targetScale[i] >= (1 - jazzAmount))
        {
            // Migration TODO: Update interface
            // setRGBBrightness(quantizerLights[targetIndex], 0.3, 0.3, 0.1);
            // setRGBBrightness(quantizer2Lights[targetIndex], 0.3, 0.3, 0.1);
        }
    }

    // Migration TODO: Set quantizer UI
    // setRGBBrightness(quantizerLights[(index1 + chordRootIndex) % 12], 0.0, 0.0, 1.0);
    // setRGBBrightness(quantizer2Lights[(index2 + chordRootIndex) % 12], 0.0, 1.0, 0);

    patch.seed.dac.WriteValue(DacHandle::Channel::ONE,
                              note1QuantizedVoltage * 819.2f);
    patch.seed.dac.WriteValue(DacHandle::Channel::TWO,
                              note2QuantizedVoltage * 819.2f);

}


void UpdateOled()
{
    patch.display.Fill(false);
    if (displayTabIndex == 0) 
    {
        std::string str  = displayLineOne;
        char*       cstr = &str[0];

        patch.display.SetCursor(0, 0);
        patch.display.WriteString(cstr, Font_7x10, true);

        patch.display.SetCursor(0, 10);
        str = displayLineTwo;
        patch.display.WriteString(cstr, Font_7x10, true);

        patch.display.SetCursor(0, 20);
        str = displayLineThree;
        patch.display.WriteString(cstr, Font_7x10, true);
    } else if (displayTabIndex == 1)
    {
        std::string str = "File browser";
        char*       cstr = &str[0];

        patch.display.SetCursor(0, 0);
        patch.display.WriteString(cstr, Font_7x10, true);

        patch.display.SetCursor(0, 10);
        str = errorMessage;
        patch.display.WriteString(cstr, Font_7x10, true);

        for (std::size_t i = 0; i < fileList.size(); i++ ) {
            patch.display.SetCursor(0, (i + 1)*10);
            string str;
            fileListCursor == i ? str += ">" : str += " ";
            str += fileList[i];
            char* cstr = &str[0];
            patch.display.WriteString(cstr, Font_7x10, true);
        }
    }

    patch.display.Update();
}

void UpdateOutputs()
{
}


// TODOs
// Handle empty SD card


