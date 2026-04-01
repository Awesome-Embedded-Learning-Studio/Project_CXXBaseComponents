#include "dir_scan.h"
#include <algorithm>
#include <iostream>
#include <queue>

DirScanner::DirScanner(size_t max_depth)
    : max_depth_(max_depth) {}

ScanResult DirScanner::scan(const fs::path& root) {
    result_ = {};
    all_files_.clear();

    try {
        if (!fs::exists(root)) {
            throw std::runtime_error("Path does not exist");
        }
        scan_impl(root, 0);

        // top 10 largest - 使用 priority_queue 实现 O(n log k) 的 top-k 算法
        constexpr size_t TOP_K = 10;
        size_t top_n = std::min<size_t>(TOP_K, all_files_.size());

        if (top_n > 0) {
            // 使用小顶堆维护当前最大的 k 个文件
            // 小顶堆：堆顶是最小的元素，方便比较和替换
            auto cmp = [](const FileInfo& a, const FileInfo& b) {
                return a.size > b.size;  // 注意：小顶堆用 > 比较
            };
            std::priority_queue<FileInfo, std::vector<FileInfo>, decltype(cmp)> min_heap(cmp);

            for (const auto& file : all_files_) {
                if (min_heap.size() < top_n) {
                    // 堆未满，直接插入
                    min_heap.push(file);
                } else if (file.size > min_heap.top().size) {
                    // 当前文件比堆顶大，替换堆顶
                    min_heap.pop();
                    min_heap.push(file);
                }
                // 否则：当前文件太小，忽略
            }

            // 将堆中元素转移到结果（堆顶最小，需要逆序）
            result_.largest_files.reserve(top_n);
            while (!min_heap.empty()) {
                result_.largest_files.push_back(min_heap.top());
                min_heap.pop();
            }
            // 堆是小顶堆，弹出顺序是从小到大，需要反转
            std::reverse(result_.largest_files.begin(), result_.largest_files.end());
        }

    } catch (const std::exception& e) {
        std::cerr << "Scan error: " << e.what() << std::endl;
    }

    return result_;
}

void DirScanner::scan_impl(const fs::path& path, size_t depth) {
    if (depth > max_depth_) return;

    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            try {
                if (entry.is_directory()) {
                    result_.dir_count++;
                    scan_impl(entry.path(), depth + 1);
                } else if (entry.is_regular_file()) {
                    add_file(entry);
                }
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Permission denied or error: " << entry.path() << "\n";
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Cannot access: " << path << "\n";
    }
}

void DirScanner::add_file(const fs::directory_entry& entry) {
    uintmax_t size = 0;
    try {
        size = entry.file_size();
    } catch (...) {
        size = 0;
    }

    result_.file_count++;
    result_.total_size += size;

    std::string ext = entry.path().extension().string();
    if (ext.empty()) ext = "[no_ext]";

    result_.ext_count[ext]++;

    all_files_.push_back({entry.path(), size});
}
