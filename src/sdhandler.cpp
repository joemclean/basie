#include "sdhandler.hpp"
#include "fatfs.h"
#include <algorithm>
#include <array>
#include <cctype>

namespace SDHandler {
std::string Card::init() {
    if (!initialized_) {
        daisy::SdmmcHandler::Config config;
        config.Defaults();
        if (sd_.Init(config) != daisy::SdmmcHandler::Result::OK) return "SD setup failed";
        if (filesystem_.Init(daisy::FatFSInterface::Config::MEDIA_SD) != daisy::FatFSInterface::OK)
            return "SD driver failed";
        initialized_ = true;
    }
    if (f_mount(&filesystem_.GetSDFileSystem(), filesystem_.GetSDPath(), 1) != FR_OK)
        return "Cannot mount SD card";
    return {};
}

FileList Card::listSongs() {
    FileList result;
    DIR directory{};
    if (f_opendir(&directory, "/") != FR_OK) return {{}, "Cannot list SD card"};
    FILINFO info{};
    for (;;) {
        if (f_readdir(&directory, &info) != FR_OK) {
            result.error = "SD directory read failed";
            result.files.clear();
            break;
        }
        if (info.fname[0] == '\0') break;
        const std::string name = info.fname;
        if ((info.fattrib & AM_DIR) != 0 || name.size() < 4) continue;
        auto extension = name.substr(name.size() - 4);
        for (auto& c : extension) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (extension == ".txt") {
            if (result.files.size() == 256) { result.error = "Too many songs (max 256)"; break; }
            result.files.push_back(name);
        }
    }
    if (f_closedir(&directory) != FR_OK && result.error.empty()) result.error = "SD directory close failed";
    if (!result.error.empty()) result.files.clear();
    std::sort(result.files.begin(), result.files.end());
    return result;
}

Songs::ParseResult Card::load(const std::string& path) {
    auto& file = file_;
    if (f_open(&file, path.c_str(), FA_READ) != FR_OK) return {{}, "Cannot open song"};
    std::string text;
    std::string error;
    if (f_size(&file) > Songs::maxFileBytes) error = "File exceeds 4095 bytes";
    std::array<char, 256> buffer{};
    while (error.empty()) {
        UINT bytes = 0;
        if (f_read(&file, buffer.data(), static_cast<UINT>(buffer.size()), &bytes) != FR_OK) {
            error = "SD song read failed";
            break;
        }
        // Also check the actual bytes: the file could have changed since f_size.
        if (text.size() + bytes > Songs::maxFileBytes) { error = "File exceeds 4095 bytes"; break; }
        text.append(buffer.data(), bytes);
        if (bytes < buffer.size()) break;
    }
    if (f_close(&file) != FR_OK && error.empty()) error = "SD song close failed";
    if (!error.empty()) return {{}, error};
    return Songs::parse(text);
}
} // namespace SDHandler
