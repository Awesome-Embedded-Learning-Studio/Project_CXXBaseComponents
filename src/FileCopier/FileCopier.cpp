#include "fcopy.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

namespace {

class ProgressBar {
public:
  explicit ProgressBar(int width = 20) : bar_width_(width) {}

  void update(std::uintmax_t copied, std::uintmax_t total,
              double speed_bytes_per_s) const {
    double fraction = (total == 0) ? 1.0 : static_cast<double>(copied) / total;
    int filled = static_cast<int>(fraction * bar_width_);

    // Build bar
    std::cout << "[";
    for (int i = 0; i < filled; ++i)
      std::cout << "=";
    if (filled < bar_width_)
      std::cout << ">";
    for (int i = filled + 1; i < bar_width_; ++i)
      std::cout << " ";
    std::cout << "] ";

    // Percent and size
    double percent = fraction * 100.0;
    double copied_mb = static_cast<double>(copied) / (1024.0 * 1024.0);
    double total_mb = static_cast<double>(total) / (1024.0 * 1024.0);

    // ETA
    double eta_seconds = 0.0;
    if (speed_bytes_per_s > 1e-6 && copied < total)
      eta_seconds = static_cast<double>(total - copied) / speed_bytes_per_s;

    std::cout << std::fixed << std::setprecision(1) << percent << "% | "
              << copied_mb << "MB/" << total_mb << "MB | "
              << (speed_bytes_per_s / (1024.0 * 1024.0)) << "MB/s | ETA: ";

    if (copied >= total) {
      std::cout << "0s";
    } else if (eta_seconds >= 3600) {
      int h = static_cast<int>(eta_seconds) / 3600;
      int m = (static_cast<int>(eta_seconds) % 3600) / 60;
      std::cout << h << "h " << m << "m";
    } else if (eta_seconds >= 60) {
      int m = static_cast<int>(eta_seconds) / 60;
      int s = static_cast<int>(eta_seconds) % 60;
      std::cout << m << "m " << s << "s";
    } else {
      int s = static_cast<int>(eta_seconds + 0.5);
      std::cout << s << "s";
    }

    // Carriage return to overwrite the same line
    std::cout << '\r' << std::flush;
  }

private:
  int bar_width_;
};

} // anonymous namespace

FileCopier::FileCopier(std::size_t chunk_size) : chunk_size_(chunk_size) {}

bool FileCopier::copy(const std::string &src_path,
                      const std::string &dst_path) {
  try {
    if (!fs::exists(src_path)) {
      std::cerr << "Source file does not exist: " << src_path << "\n";
      return false;
    }

    std::uintmax_t total_size = fs::file_size(src_path);

    std::ifstream in(src_path, std::ios::binary);
    if (!in) {
      std::cerr << "Failed to open source file for reading: " << src_path
                << "\n";
      return false;
    }

    std::ofstream out(dst_path, std::ios::binary | std::ios::trunc);
    if (!out) {
      std::cerr << "Failed to open destination file for writing: " << dst_path
                << "\n";
      return false;
    }

    std::vector<char> buffer(chunk_size_);
    std::uintmax_t copied = 0;

    auto t_start = std::chrono::steady_clock::now();
    auto last_report = t_start;

    // If file is empty, just touch destination
    ProgressBar bar;
    if (total_size == 0) {
      out.close();
      bar.update(0, 0, 0.0);
      std::cout << "\n";
      return true;
    }

    while (in) {
      in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
      std::streamsize read_bytes = in.gcount();
      if (read_bytes <= 0)
        break;

      out.write(buffer.data(), read_bytes);
      if (!out) {
        std::cerr << "Write error while writing to: " << dst_path << "\n";
        return false;
      }

      copied += static_cast<std::uintmax_t>(read_bytes);

      // Update progress at most 10 times per second
      auto now = std::chrono::steady_clock::now();
      std::chrono::duration<double> since_last = now - last_report;
      if (since_last.count() >= 0.1 || copied == total_size) {
        std::chrono::duration<double> elapsed = now - t_start;
        double speed = (elapsed.count() > 1e-9)
                           ? (static_cast<double>(copied) / elapsed.count())
                           : 0.0;
        bar.update(copied, total_size, speed);
        last_report = now;
      }
    }

    // Ensure data flushed
    out.flush();
    out.close();
    in.close();

    // Final progress line (complete)
    auto t_end = std::chrono::steady_clock::now();
    std::chrono::duration<double> total_elapsed = t_end - t_start;
    double avg_speed =
        (total_elapsed.count() > 1e-9)
            ? (static_cast<double>(copied) / total_elapsed.count())
            : 0.0;
    bar.update(copied, total_size, avg_speed);
    std::cout << "\n";

    // Verify sizes
    std::uintmax_t dst_size = fs::file_size(dst_path);
    if (dst_size != total_size) {
      std::cerr << "Size mismatch after copy. src=" << total_size
                << " dst=" << dst_size << "\n";
      return false;
    }

    return true;
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Filesystem error: " << e.what() << "\n";
    return false;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return false;
  }
}
