#pragma once

#include <cstddef>
#include <string>

class FileCopier {
public:
  explicit FileCopier(std::size_t chunk_size = 8 * 1024);

  // Copy src -> dst, return true on success.
  bool copy(const std::string &src_path, const std::string &dst_path);

  // Optional: change chunk size before copy
  void setChunkSize(std::size_t size) { chunk_size_ = size; }

private:
  std::size_t chunk_size_;
};
