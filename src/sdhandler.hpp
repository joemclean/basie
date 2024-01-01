#ifndef SDHANDLER_H
#define SDHANDLER_H

#include <string>
#include <vector>

using std::string;
using std::vector;

void initSDCard();

vector<string> listTxtFiles(const char* path);

string loadChordsFromFile(string filePath);

#endif