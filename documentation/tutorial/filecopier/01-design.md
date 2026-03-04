# 01 - 类设计与分块读写

## 从最简单的接口开始

在设计一个类的时候，我喜欢先从使用者的角度出发。想象一下，如果我是使用这个库的人，我希望怎么调用它？

最理想的情况是：创建对象，调用拷贝方法，拿到结果。就像这样：

```cpp
FileCopier copier;
if (copier.copy("source.txt", "dest.txt")) {
    std::cout << "Success!\n";
} else {
    std::cout << "Failed!\n";
}
```

简单直接，不需要读几百页文档就能上手。基于这个想法，我们的类设计非常简洁：

```cpp
class FileCopier {
public:
  explicit FileCopier(std::size_t chunk_size = 8 * 1024);
  bool copy(const std::string &src_path, const std::string &dst_path);
  void setChunkSize(std::size_t size) { chunk_size_ = size; }

private:
  std::size_t chunk_size_;
};
```

构造函数接受一个可选的块大小参数，默认 8KB。`copy()` 方法执行实际的拷贝操作，返回布尔值表示成功或失败。`setChunkSize()` 允许在拷贝前调整块大小。

`explicit` 关键字很重要，它防止了隐式类型转换。没有 `explicit`，`FileCopier copier = 1024;` 这样的代码也能编译通过，但这通常不是我们想要的语义。

### copy() 方法的基本结构

现在让我们实现 `copy()` 方法。它的基本流程是：

1. 检查源文件是否存在
2. 获取文件大小（用于计算进度）
3. 打开源文件和目标文件
4. 分块读取和写入
5. 关闭文件并验证

```cpp
bool FileCopier::copy(const std::string &src_path,
                      const std::string &dst_path) {
  try {
    // 1. 检查源文件
    if (!fs::exists(src_path)) {
      std::cerr << "Source file does not exist: " << src_path << "\n";
      return false;
    }

    // 2. 获取文件大小
    std::uintmax_t total_size = fs::file_size(src_path);

    // 3. 打开文件
    std::ifstream in(src_path, std::ios::binary);
    if (!in) {
      std::cerr << "Failed to open source file for reading: " << src_path << "\n";
      return false;
    }

    std::ofstream out(dst_path, std::ios::binary | std::ios::trunc);
    if (!out) {
      std::cerr << "Failed to open destination file for writing: " << dst_path << "\n";
      return false;
    }

    // 4. 分块拷贝（下面详细讲解）
    // ...

    // 5. 验证
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

### 文件系统操作的细节

这里我们用了 C++17 的 `std::filesystem`（简写为 `fs`）。`fs::exists()` 检查文件是否存在，`fs::file_size()` 获取文件大小。

注意 `fs::file_size()` 返回的是 `std::uintmax_t`，这是在 `<cstdint>` 头文件中定义的无符号整数类型，它是最大的无符号整数类型，能表示系统支持的最大文件大小。用 `size_t` 可能在处理超大文件时溢出。

打开文件时我们用了 `std::ios::binary` 标志，这告诉流不要做任何字符转换（比如 Windows 上的 `\n` 到 `\r\n` 转换）。对于文件拷贝，我们总是应该用二进制模式。

对于输出文件，我们还加了 `std::ios::trunc` 标志，这表示如果目标文件已存在就截断它（清空内容）。没有这个标志，如果目标文件已存在且比源文件大，拷贝后目标文件会保留原来的尾部数据。

### 分块读写循环

核心的拷贝逻辑在 while 循环中：

```cpp
std::vector<char> buffer(chunk_size_);
std::uintmax_t copied = 0;

while (in) {
  // 读取一块
  in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
  std::streamsize read_bytes = in.gcount();

  if (read_bytes <= 0) break;

  // 写入这块
  out.write(buffer.data(), read_bytes);
  if (!out) {
    std::cerr << "Write error while writing to: " << dst_path << "\n";
    return false;
  }

  copied += static_cast<std::uintmax_t>(read_bytes);
}
```

这里有几个关键点：

首先，我们用 `buffer.data()` 获取指向底层数组的指针。`std::vector` 保证内存是连续的，所以可以直接传给 C 风格的 `read()` 函数。

其次，`static_cast<std::streamsize>` 是必要的，因为 `read()` 的参数类型是 `std::streamsize`（通常是 `long`），而 `buffer.size()` 是 `size_t`（通常是 `unsigned long`）。编译器会警告类型不匹配，所以我们显式转换。

第三，`in.gcount()` 返回上一次 `read()` 实际读取的字节数。这个数字可能小于请求的字节数，因为可能到了文件末尾。每次调用 `read()` 后应该立即调用 `gcount()`，因为它的值会被下一次 I/O 操作覆盖。

第四，我们检查 `read_bytes <= 0` 而不是 `== 0`，因为理论上 `gcount()` 可能返回 -1 表示错误。虽然很少见，但防御性编程总是好的。

第五，写入时我们也检查了 `out` 的状态。`write()` 可能因为磁盘满、权限问题等原因失败，但我们不能只假设它成功了。

### 为什么用 vector 而不是数组？

你可能会问，为什么不用 `char buffer[8192]` 这种 C 风格数组？有几个原因：

1. **动态大小**：`vector` 的大小可以在运行时决定，C 数组必须在编译时确定
2. **RAII**：`vector` 会自动管理内存，不需要手动 `new` 和 `delete`
3. **异常安全**：如果发生异常，`vector` 会自动析构并释放内存
4. **零初始化**：`vector<char>(n)` 会初始化所有字节为 0，对于文件拷贝来说这没影响，但能避免某些情况下的未定义行为

### 处理空文件

有个边界情况需要特殊处理：空文件。如果源文件大小是 0，我们的 while 循环一次都不会执行，但目标文件还是会被创建（因为我们用 `std::ios::trunc` 打开了）。这是正确的行为，但我们还是显式处理一下：

```cpp
if (total_size == 0) {
  out.close();
  return true;
}
```

这看起来多余，但处理边界情况的良好习惯能避免很多诡异的 bug。

### 当前的完整实现

把上面的代码拼起来，我们就有了一个基本可用的 FileCopier：

```cpp
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
      std::cerr << "Failed to open source file for reading: " << src_path << "\n";
      return false;
    }

    std::ofstream out(dst_path, std::ios::binary | std::ios::trunc);
    if (!out) {
      std::cerr << "Failed to open destination file for writing: " << dst_path << "\n";
      return false;
    }

    std::vector<char> buffer(chunk_size_);
    std::uintmax_t copied = 0;

    if (total_size == 0) {
      out.close();
      return true;
    }

    while (in) {
      in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
      std::streamsize read_bytes = in.gcount();
      if (read_bytes <= 0) break;

      out.write(buffer.data(), read_bytes);
      if (!out) {
        std::cerr << "Write error while writing to: " << dst_path << "\n";
        return false;
      }

      copied += static_cast<std::uintmax_t>(read_bytes);
    }

    out.flush();
    out.close();
    in.close();

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

### 测试一下

现在可以编译测试一下：

```bash
$ g++ -std=c++17 -o fcopy demo.cpp FileCopier.cpp
$ ./fcopy source.txt dest.txt
$
$ echo $?
0
```

成功了！...等等，什么都没输出？这是因为我们还没有实现进度条，拷贝过程是静默的。对于小文件这可能还好，但对于大文件，用户会想看进度。

下一章我们就来实现进度条，这是整个项目最有意思的部分。我们会学习如何控制终端输出、如何计算速度、如何预估剩余时间。准备好了吗？

[下一章：进度条实现 →](02-progressbar.md)
