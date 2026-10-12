#pragma once
#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <vector>
using UINT = unsigned;
using FRESULT = int;
constexpr int FR_OK = 0, FA_READ = 1, AM_DIR = 16;
struct FIL { std::string path; std::size_t offset = 0; };
struct DIR { std::size_t index = 0; };
struct FILINFO { char fname[256]{}; int fattrib = 0; };
namespace FakeFs {
inline std::map<std::string, std::string> files;
inline std::vector<std::string> names;
inline bool mountFailure = false, openFailure = false, readFailure = false, listFailure = false;
inline bool directoryReadFailure = false, closeFailure = false;
inline int closedFiles = 0;
inline int sizeOverride = -1;
}
inline int f_mount(int*, const char*, int) { return FakeFs::mountFailure ? 1 : FR_OK; }
inline int f_opendir(DIR* dir, const char*) { dir->index = 0; return FakeFs::listFailure ? 1 : FR_OK; }
inline int f_readdir(DIR* dir, FILINFO* info) {
    if (FakeFs::directoryReadFailure) return 1;
    info->fname[0] = 0;
    if (dir->index < FakeFs::names.size()) {
        std::strncpy(info->fname, FakeFs::names[dir->index++].c_str(), sizeof(info->fname) - 1);
        info->fname[sizeof(info->fname) - 1] = 0;
    }
    return FR_OK;
}
inline int f_closedir(DIR*) { return FR_OK; }
inline int f_open(FIL* file, const char* path, int) {
    if (FakeFs::openFailure || FakeFs::files.count(path) == 0) return 1;
    file->path = path; file->offset = 0;
    return FR_OK;
}
inline std::size_t f_size(FIL* file) {
    return FakeFs::sizeOverride >= 0 ? static_cast<std::size_t>(FakeFs::sizeOverride) : FakeFs::files[file->path].size();
}
inline int f_read(FIL* file, void* output, UINT capacity, UINT* bytes) {
    if (FakeFs::readFailure) return 1;
    const auto& data = FakeFs::files[file->path];
    *bytes = static_cast<UINT>(std::min<std::size_t>(capacity, data.size() - file->offset));
    std::memcpy(output, data.data() + file->offset, *bytes);
    file->offset += *bytes;
    return FR_OK;
}
inline int f_close(FIL*) { ++FakeFs::closedFiles; return FakeFs::closeFailure ? 1 : FR_OK; }
