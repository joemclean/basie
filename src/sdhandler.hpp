#ifndef SDHANDLER_H
#define SDHANDLER_H

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace SDHandler {

  void initSDCard();

  vector<string> listTxtFiles(const char* path);

  string loadChordsFromFile(string filePath);

  string trim(const string& str);

  vector<string> parseChords(const string& chordString);

}

#endif