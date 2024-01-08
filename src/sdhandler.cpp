#include "daisysp.h"
#include "daisy_patch.h"

#include "sdhandler.hpp"
#include "fatfs.h"

using std::string;
using std::vector;

using namespace daisy;
using namespace daisysp;

SdmmcHandler   sd;
FatFSInterface fsi;

DIR dir;
FILINFO fno;

FIL file;       // File object
FRESULT fr;     // FatFs return code
UINT br;        // Read count

constexpr size_t bufferSize = 4096; // Example size, adjust as needed
char buffer[bufferSize] = {0};

void initSDCard() 
{
    // Init SD Card
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sd.Init(sd_cfg);

    // Links libdaisy i/o to fatfs driver.
    fsi.Init(FatFSInterface::Config::MEDIA_SD);

    // Mount SD Card
    f_mount(&fsi.GetSDFileSystem(), "/", 1);
}

vector<string> listTxtFiles(const char* path) {

    vector<string> txtFiles;

    fr = f_opendir(&dir, path);  // Open the directory
    if (fr == FR_OK) {
        for (;;) {
            fr = f_readdir(&dir, &fno);  // Read a directory item
            if (fr != FR_OK || fno.fname[0] == 0) break;  // Break on error or end of dir
            if (fno.fattrib & AM_DIR) {
                // It's a directory, you can ignore or handle accordingly
            } else { 
                // It's a file, check if it's a .txt file
                string fileName = fno.fname;
                if (fileName.size() >= 4 && fileName.substr(fileName.size() - 4) == ".txt") {
                    txtFiles.push_back(fileName);
                }
            }
        }
        f_closedir(&dir);
    }
    return txtFiles;
}

string loadChordsFromFile(string filePath)
{
    string chordData;

    // Try to open the file
    fr = f_open(&file, filePath.c_str(), FA_READ);

    if (fr == FR_OK) {
        // Read the entire file into the buffer
        fr = f_read(&file, buffer, bufferSize - 1, &br);
        if (fr == FR_OK) {
            // Ensure there's a null terminator at the end of the read data
            buffer[br] = '\0';
            
            // Now 'buffer' contains the entire file content as a C-style string
            string fileContent(buffer);
            chordData = fileContent;

        }
        f_close(&file);
    }

    return chordData;
}


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