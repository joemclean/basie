#include "daisysp.h"
#include "daisy_patch.h"

#include "src/theory.hpp"
#include "src/quantizer.hpp"
#include "src/sdhandler.hpp"
#include "src/display.hpp"
#include "src/MIDI.hpp"

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

// --- System variables --- //

// Display variables
vector<string> fileList = {};
int loadedFileIndex = 0;

int displayTabIndex = 0;
bool encoderIsHeld = false;
bool tabChangeInProcess = false;

string chordDisplayName = "No chord";
string currentFileName = "No file";

int fileListCursor = 0;

// Module state variables
vector<string> currentSongChords = {};
int playhead = 0;

bool beatChanging = false;

int chordType = 0;
int chordRootIndex = 0;

Theory::Chord *targetChord;
array<float, 12> targetScale = Theory::chordList[0]->chordScale;

float jazzAmountCh1 = 0.5;
float jazzAmountCh2 = 0.5;

float note1QuantizedVoltage = 0.0;
int note1index = 0;
float note2QuantizedVoltage = 0.0;
int note2index = 0;

void UpdateControls();
void Process();
void UpdateOled();
void UpdateOutputs();

void loadSong(const string& fileName) {
  MIDI::clearMidi();
  string fileChords = SDHandler::loadChordsFromFile(fileName);
  currentSongChords = SDHandler::parseChords(fileChords);
  currentFileName = fileName;
  playhead = 0;
}

void ProcessEncoder() {
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
  if (increment != 0) {
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
}

void UpdateControls() {
  patch.ProcessAnalogControls();
  patch.ProcessDigitalControls();
  ProcessEncoder();
}

Oscillator osc[3];

float testval;

void SetupOsc(float samplerate)
{
    for(int i = 0; i < 3; i++)
    {
        osc[i].Init(samplerate);
        osc[i].SetAmp(.7);
    }
}

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        float mix = 0;
        //Process and output the three oscillators
        for(size_t chn = 0; chn < 3; chn++)
        {
            float sig = osc[chn].Process();
            mix += sig * .25f;
            out[chn][i] = sig;
        }

        // output the mixed oscillators
        out[3][i] = mix;
    }
}

// --- Implementation --- //

int main(void) {
    float samplerate;
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    samplerate = patch.AudioSampleRate();

    SetupOsc(samplerate);

    testval = 0.f;

    patch.StartAdc();
    patch.StartAudio(AudioCallback);

    for(int i = 0; i < 3; i++)
    {
        osc[i].SetFreq(440);
        osc[i].SetWaveform(0);
    }

  // start callback
  patch.StartAudio(AudioCallback);

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

  chordDisplayName = chordString;

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
      MIDI::MIDISendNoteOn(0, targetMidiNote, 100, i);
      if (i < 4) {
        if (i > 0) {
          targetMidiNote = targetMidiNote + 12;
        }
        MIDI::MIDISendNoteOn(0, targetMidiNote, 100, i);
      }
      if (i < 3){
        float detune = i * 0.1f;
        osc[i].SetFreq(mtof(targetMidiNote) - detune);
      }
    }
  }
  
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
  std::pair<float, int> values1 = Quantizer::quantizeToScale(voice1Voltage, chordRootOffsetVoltage, targetScale, jazzAmountCh1);
  std::pair<float, int> values2 = Quantizer::quantizeToScale(voice2Voltage, chordRootOffsetVoltage, targetScale, jazzAmountCh2);

  note1QuantizedVoltage = values1.first;
  note1index = values1.second;
  note2QuantizedVoltage = values2.first;
  note2index = values2.second;

  beatChanging = false;
}

void UpdateOled() {
  patch.display.Fill(false);
  if (displayTabIndex == 0) {
    Display::renderSongView(
    currentFileName,
    chordDisplayName,
    playhead,
    currentSongChords.size(),
    jazzAmountCh1,
    jazzAmountCh2,
    chordRootIndex,
    targetScale,
    note1index,
    note2index
  );
  } else if (displayTabIndex == 1) {
    Display::renderFileBrowser(
      fileList,
      loadedFileIndex,
      fileListCursor
    );
  }
  patch.display.Update();
}

void UpdateOutputs() {
  patch.seed.dac.WriteValue(DacHandle::Channel::ONE, note1QuantizedVoltage * 819.2f);
  patch.seed.dac.WriteValue(DacHandle::Channel::TWO, note2QuantizedVoltage * 819.2f);
}
