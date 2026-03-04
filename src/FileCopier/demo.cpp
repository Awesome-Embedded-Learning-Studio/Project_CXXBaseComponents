#include "fcopy.h"

#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: fcopy <source> <destination>\n";
    std::cerr << "  source       Path to the source file\n";
    std::cerr << "  destination  Path to the destination file\n";
    return 1;
  }

  std::string src_path = argv[1];
  std::string dst_path = argv[2];

  FileCopier copier;
  if (!copier.copy(src_path, dst_path)) {
    std::cerr << "Copy failed: " << src_path << " -> " << dst_path << "\n";
    return 1;
  }

  std::cout << "Copy completed successfully.\n";
  return 0;
}
