#include "daisy_patch.h"

#include "src/theory.hpp"
#include "src/quantizer.hpp"
#include "src/sdhandler.hpp"
#include "src/display.hpp"
#include "src/MIDI.hpp"

daisy::DaisyPatch patch;

// --- System variables --- //

// Display variables
vector<string> fileList;
int loadedFileIndex = 0;

int displayTabIndex = 0;
bool encoderIsHeld = false;
bool tabChangeInProcess = false;

string chordDisplayName;

// Module state variables
vector<string> currentSongChords;
int playhead = 0;

bool beatChanging = false;

int chordType = 0;
int chordRootIndex = 0;

Theory::Chord *targetChord;
array<float, 12> targetScale;

float jazzAmountCh1 = 0.5;
float jazzAmountCh2 = 0.5;

float note1QuantizedVoltage = 0.0;
float note2QuantizedVoltage = 0.0;

void UpdateControls();
void Process();
void UpdateOled();
void UpdateOutputs();

void loadSong(const string& fileName) {
  MIDI::clearMidi();
  string fileChords = SDHandler::loadChordsFromFile(fileName);
  currentSongChords = SDHandler::parseChords(fileChords);
  Display::displayLineOne = fileName;
  playhead = 0;
}

void ProcessEncoder() {
  if (patch.encoder.RisingEdge()) {
    encoderIsHeld = true;
  }
  if (patch.encoder.FallingEdge()) {
    encoderIsHeld = false;
    if (!tabChangeInProcess && displayTabIndex == 1) {
      loadSong(fileList[Display::fileListCursor]);
      loadedFileIndex = Display::fileListCursor;
    }
    tabChangeInProcess = false;
  }
  int increment = patch.encoder.Increment();
  if (increment != 0) {
    if (encoderIsHeld) {
      // Change tab
      displayTabIndex++;
      displayTabIndex = displayTabIndex % 2;
      tabChangeInProcess = true;
    } else {
      // Navigate file system
      int fileListSize = static_cast<int>(fileList.size());
      Display::fileListCursor += increment;

      // Wrap around logic
      if (Display::fileListCursor >= fileListSize) {
        Display::fileListCursor = 0;
      } else if (Display::fileListCursor < 0) {
        Display::fileListCursor = fileListSize > 0 ? fileListSize - 1 : 0;
      }
    }
  }
}

void UpdateControls() {
  patch.ProcessAnalogControls();
  patch.ProcessDigitalControls();
  ProcessEncoder();
}

// --- Implementation --- //

int main(void) {
  patch.Init(); // Initialize hardware (daisy seed, and patch)
  patch.StartAdc();

  SDHandler::initSDCard();
  fileList = SDHandler::listTxtFiles("/");
  loadSong(fileList[0]);

  while(1) {
    UpdateControls();
    UpdateOled();
    Process();
    UpdateOutputs();
  }
}

void Process() {                                                                                                                   
  // Advance the playhead on clock trigger
  if (patch.gate_input[0].Trig()) {
    beatChanging = true;
    int songLength = currentSongChords.size();
    playhead = (playhead + 1) % songLength;
  }

  // Reset the playhead on reset trigger
  if (patch.gate_input[1].Trig()) {
    playhead = 0;
  }

    // TODO - only bother processing all of this if the playhead advances
  string chordString = currentSongChords[playhead];

  Display::displayLineTwo = "Chord " + std::to_string(playhead + 1) + "/" + std::to_string(currentSongChords.size()) + ":";
  Display::displayLineThree = chordString;

  // Parse the chord string

  string chordRoot = "C";
  string chordType = "maj7";

  char chordBuffer[10];
  strcpy(chordBuffer, chordString.c_str());

  char chordRootBuffer[10];
  char chordTypeBuffer[10];

  strcpy(chordRootBuffer, strtok(chordBuffer, " "));
  strcpy(chordTypeBuffer, strtok(NULL, " "));

  chordRoot = chordRootBuffer;
  chordType = chordTypeBuffer;

  // Match the root
  for (int i = 0; i < 12; i++) {
    if (chordRoot == Theory::noteNamesFlats[i] || chordRoot == Theory::noteNamesSharps[i]) {
      chordRootIndex = i;
    }
  }

  // Match the chord type
  std::array<Theory::Chord*, Theory::chordList.size()> availableChordList = Theory::chordList;
  targetChord = availableChordList[0]; // initialize with default
  for (size_t i = 0; i < availableChordList.size(); i++) {
    if (availableChordList[i]->displayName == chordType) {
      targetChord = availableChordList[i];
    }
  }

  // Iterate over active chord tones and output as MIDI notes
  int chordToneCount = targetChord->tones.size();
  if (beatChanging) {
    MIDI::clearMidi();
    for (int i = 0; i < chordToneCount; i++) {   
      int chordTone = targetChord->tones[i];  
      // Migration TODO: Reimplement voicing type
      int targetMidiNote = 36 + chordRootIndex + chordTone;
      if (i < 4) {
        if (i > 0) {
          targetMidiNote = targetMidiNote + 12;
        }
        MIDI::MIDISendNoteOn(0, targetMidiNote, 100, i);
      }
    }
  }
  
  // Quantize input to output
  targetScale = targetChord->chordScale;

  jazzAmountCh1 = patch.GetKnobValue((daisy::DaisyPatch::Ctrl)2) * 1.f;
  jazzAmountCh2 = patch.GetKnobValue((daisy::DaisyPatch::Ctrl)3) * 1.f;

  if (jazzAmountCh1 > 1) jazzAmountCh1 = 1;
  if (jazzAmountCh1 < 0) jazzAmountCh1 = 0;

  if (jazzAmountCh2 > 1) jazzAmountCh2 = 1;
  if (jazzAmountCh2 < 0) jazzAmountCh2 = 0;

  // Read quantizer ins
  float voice1Voltage = patch.GetKnobValue((daisy::DaisyPatch::Ctrl)0) * 5.f;
  float voice2Voltage = patch.GetKnobValue((daisy::DaisyPatch::Ctrl)1) * 5.f;

  // Quantize the inputs to active notes in the target scale
  float chordRootOffsetVoltage = (float)chordRootIndex / 12.0;
  std::pair<float, int> values1 = Quantizer::quantizeToScale(voice1Voltage, chordRootOffsetVoltage, targetScale, jazzAmountCh1);
  std::pair<float, int> values2 = Quantizer::quantizeToScale(voice2Voltage, chordRootOffsetVoltage, targetScale, jazzAmountCh2);

  note1QuantizedVoltage = values1.first;
  note2QuantizedVoltage = values2.first;

  beatChanging = false;
}

void UpdateOled() {
  patch.display.Fill(false);
  if (displayTabIndex == 0) {
    Display::renderSongView(
    jazzAmountCh1,
    jazzAmountCh2,
    chordRootIndex,
    targetScale
  );
  } else if (displayTabIndex == 1) {
    Display::renderFileBrowser(
      fileList,
      loadedFileIndex
    );
  }
  patch.display.Update();
}

void UpdateOutputs() {
  patch.seed.dac.WriteValue(daisy::DacHandle::Channel::ONE, note1QuantizedVoltage * 819.2f);
  patch.seed.dac.WriteValue(daisy::DacHandle::Channel::TWO, note2QuantizedVoltage * 819.2f);
}
