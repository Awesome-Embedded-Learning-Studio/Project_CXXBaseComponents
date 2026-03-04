# 03 - 完整流程与验证

## 代码组装

前面我们已经分别实现了文件拷贝的核心逻辑和进度条组件。现在让我们把它们组装成一个完整的、可用的 FileCopier。

### 完整的 FileCopier.cpp

```cpp
#include "fcopy.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

namespace {

// ================================================================
// ProgressBar - 内部实现类
// ================================================================
// 这个类不对外暴露，只在 FileCopier 内部使用
// 目的是保持 FileCopier 的公共接口简洁
// ================================================================

class ProgressBar {
public:
  explicit ProgressBar(int width = 20) : bar_width_(width) {}

  void update(std::uintmax_t copied, std::uintmax_t total,
              double speed_bytes_per_s) const {
    double fraction = (total == 0) ? 1.0 : static_cast<double>(copied) / total;
    int filled = static_cast<int>(fraction * bar_width_);

    // 构建进度条
    std::cout << "[";
    for (int i = 0; i < filled; ++i)
      std::cout << "=";
    if (filled < bar_width_)
      std::cout << ">";
    for (int i = filled + 1; i < bar_width_; ++i)
      std::cout << " ";
    std::cout << "] ";

    // 百分比和大小
    double percent = fraction * 100.0;
    double copied_mb = static_cast<double>(copied) / (1024.0 * 1024.0);
    double total_mb = static_cast<double>(total) / (1024.0 * 1024.0);

    // ETA 计算
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

    // 回车覆盖，不换行
    std::cout << '\r' << std::flush;
  }

private:
  int bar_width_;
};

} // anonymous namespace

// ================================================================
// FileCopier 实现
// ================================================================

FileCopier::FileCopier(std::size_t chunk_size)
    : chunk_size_(chunk_size) {}

bool FileCopier::copy(const std::string &src_path,
                      const std::string &dst_path) {
  try {
    // 检查源文件
    if (!fs::exists(src_path)) {
      std::cerr << "Source file does not exist: " << src_path << "\n";
      return false;
    }

    std::uintmax_t total_size = fs::file_size(src_path);

    // 打开源文件
    std::ifstream in(src_path, std::ios::binary);
    if (!in) {
      std::cerr << "Failed to open source file for reading: " << src_path << "\n";
      return false;
    }

    // 打开目标文件
    std::ofstream out(dst_path, std::ios::binary | std::ios::trunc);
    if (!out) {
      std::cerr << "Failed to open destination file for writing: " << dst_path << "\n";
      return false;
    }

    std::vector<char> buffer(chunk_size_);
    std::uintmax_t copied = 0;

    auto t_start = std::chrono::steady_clock::now();
    auto last_report = t_start;

    ProgressBar bar;

    // 处理空文件
    if (total_size == 0) {
      out.close();
      bar.update(0, 0, 0.0);
      std::cout << "\n";
      return true;
    }

    // 分块拷贝循环
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

      // 每 0.1 秒或拷贝完成时更新进度
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

    // 确保数据刷新
    out.flush();
    out.close();
    in.close();

    // 最终进度显示
    auto t_end = std::chrono::steady_clock::now();
    std::chrono::duration<double> total_elapsed = t_end - t_start;
    double avg_speed =
        (total_elapsed.count() > 1e-9)
            ? (static_cast<double>(copied) / total_elapsed.count())
            : 0.0;
    bar.update(copied, total_size, avg_speed);
    std::cout << "\n";

    // 验证文件大小
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
```

### 编写测试程序

现在我们写一个简单的测试程序：

```cpp
#include "fcopy.h"
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " <source> <destination>\n";
    return 1;
  }

  FileCopier copier(64 * 1024);  // 64KB 块大小

  std::cout << "Copying " << argv[1] << " to " << argv[2] << "...\n";

  if (copier.copy(argv[1], argv[2])) {
    std::cout << "Copy completed successfully!\n";
    return 0;
  } else {
    std::cerr << "Copy failed!\n";
    return 1;
  }
}
```

### 编译和运行

```bash
$ g++ -std=c++17 -O2 -o fcopy demo.cpp FileCopier.cpp
```

注意 `-O2` 优化开关，对于 I/O 密集型程序，编译器优化能带来显著性能提升。

### 测试不同场景

**测试 1：小文件**

```bash
$ echo "Hello, World!" > test.txt
$ ./fcopy test.txt test_copy.txt
Copying test.txt to test_copy.txt...
[====================] 100.0% | 0.0MB/0.0MB | 0.0MB/s | ETA: 0s
Copy completed successfully!

$ diff test.txt test_copy.txt
$ echo $?
0
```

`diff` 没有输出且返回 0，说明文件完全相同。

**测试 2：大文件（创建 100MB 测试文件）**

```bash
$ dd if=/dev/zero of=large.bin bs=1M count=100
100+0 records in
100+0 records out
104857600 bytes (100 MB) copied, 0.123 s, 852 MB/s

$ ./fcopy large.bin large_copy.bin
Copying large.bin to large_copy.bin...
[====================] 100.0% | 100.0MB/100.0MB | 245.3MB/s | ETA: 0s
Copy completed successfully!
```

进度条实时更新，显示当前速度和 ETA。

**测试 3：空文件**

```bash
$ touch empty.txt
$ ./fcopy empty.txt empty_copy.txt
Copying empty.txt to empty_copy.txt...
[====================] 100.0% | 0.0MB/0.0MB | 0.0MB/s | ETA: 0s
Copy completed successfully!
```

**测试 4：错误处理**

```bash
$ ./fcopy nonexistent.txt output.txt
Copying nonexistent.txt to output.txt...
Source file does not exist: nonexistent.txt
Copy failed!
```

### 性能对比

让我们和系统自带的 `cp` 命令对比一下性能：

```bash
$ time cp large.bin large_cp.bin
real    0m0.421s
user    0m0.012s
sys     0m0.409s

$ time ./fcopy large.bin large_fcopy.bin
real    0m0.438s
user    0m0.034s
sys     0m0.404s
```

可以看到，我们的实现和系统 `cp` 的性能差距不大（`cp` 做了一些额外的优化）。考虑到我们还有进度显示的开销，这个结果是相当不错的。

### 可能的改进

如果你觉得这个 FileCopier 还不够完美，这里有一些改进方向：

1. **多线程拷贝**：用多个线程分别读写不同的文件区域
2. **稀疏文件支持**：检测并正确处理稀疏文件（空洞文件）
3. **权限保留**：拷贝文件权限和时间戳
4. **断点续传**：支持中断后继续拷贝
5. **校验和验证**：用 MD5/SHA256 验证文件完整性

但这些已经是更高级的功能了，对于学习目的来说，我们当前的实现已经足够完整。

### 小结

恭喜你！现在你有了一个功能完整、带进度显示的文件拷贝工具。在这个过程中，我们学到了：

1. 文件 I/O 的基本操作和注意事项
2. `std::chrono` 库的使用方法
3. 终端进度条的实现技巧
4. 错误处理和边界情况的考虑
5. 性能优化的基本思路

更重要的是，我们不只是写出了一个能用的工具，更理解了每一行代码背后的原理。这种深入的理解，才是学习编程最有价值的部分。

---

## 完结撒花

现在去试试拷贝一些大文件吧，看着进度条从 0% 走到 100%，还是很有成就感的。如果你有其他需要拷贝的任务，也可以直接用这个 FileCopier，它简单可靠，而且你知道它在做什么。

[← 返回教程首页](README.md)
