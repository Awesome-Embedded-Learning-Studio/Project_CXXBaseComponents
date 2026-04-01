// ============================
// DirScanner.h
// ============================
#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

struct FileInfo {
    fs::path path;
    uintmax_t size;
};

struct ScanResult {
    size_t file_count = 0;
    size_t dir_count = 0;
    uintmax_t total_size = 0;
    std::unordered_map<std::string, size_t> ext_count;
    std::vector<FileInfo> largest_files;
};

class DirScanner {
public:
    DirScanner(size_t max_depth = SIZE_MAX);

    ScanResult scan(const fs::path& root);

private:
    void scan_impl(const fs::path& path, size_t depth);
    void add_file(const fs::directory_entry& entry);

private:
    size_t max_depth_;
    ScanResult result_;
    std::vector<FileInfo> all_files_;
};
