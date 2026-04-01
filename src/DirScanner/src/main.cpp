#include "ArgParser.h"
#include "dir_scan.h"
#include <iomanip>
#include <iostream>

void print_table(const ScanResult &r) {
  std::cout << "\n==== Scan Summary ====\n";
  std::cout << "Files: " << r.file_count << "\n";
  std::cout << "Dirs : " << r.dir_count << "\n";
  std::cout << "Size : " << r.total_size << " bytes\n";

  std::cout << "\n-- Extension Stats --\n";
  for (const auto &[ext, count] : r.ext_count) {
    std::cout << std::setw(10) << ext << " : " << count << "\n";
  }

  std::cout << "\n-- Top 10 Largest Files --\n";
  for (const auto &f : r.largest_files) {
    std::cout << f.size << "\t" << f.path.string() << "\n";
  }
}

int main(int argc, char *argv[]) {
  cc::ArgParser parser;

  parser.add_argument("-p", "--path", "Directory path to scan", false);
  parser.add_argument("-d", "--depth", "Max scan depth (0 = unlimited)", false);
  parser.parse(argc, argv);

  std::string path = ".";
  try {
    std::string p = parser.get<std::string>("path");
    if (!p.empty())
      path = p;
  } catch (...) {
  }

  size_t depth = SIZE_MAX;
  try {
    depth = parser.get<size_t>("depth");
  } catch (...) {
  }

  DirScanner scanner(depth);
  auto result = scanner.scan(path);

  print_table(result);

  return 0;
}
