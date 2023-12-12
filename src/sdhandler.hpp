#ifndef SDHANDLER_H
#define SDHANDLER_H

#include <string>
#include <vector>

std::vector<std::string> listTxtFiles(const char* path);

std::string loadChordsFromFile(std::string filePath);

#endif