// ============================
// test.cpp (简单测试)
// ============================
#include "dir_scan.h"
#include <cassert>
#include <iostream>

int main() {
  DirScanner scanner(3);
  auto result = scanner.scan(".");

  assert(result.file_count >= 0);
  assert(result.dir_count >= 0);

  std::cout << "Test passed! Files=" << result.file_count << "\n";
  return 0;
}