# 02 - 进度条实现

## 终端的魔法

进度条看似简单，但实现起来有不少细节需要考虑。核心问题是如何在终端上实现"原地更新"——每次更新都在同一行覆盖之前的内容，而不是不断打印新行。

### `\r` 和 `std::flush` 的秘密

实现原地更新需要两个关键要素：回车符 `\r` 和 `std::flush`。

- `\r`（回车符）让光标回到行首，不换行
- `std::flush` 强制刷新输出缓冲区，确保内容立即显示

让我们看个简单的例子：

```cpp
std::cout << "Loading";
for (int i = 0; i < 3; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "\rLoading" << std::string(i + 1, '.') << std::flush;
}
std::cout << "\nDone!\n";
```

这会显示：
```
Loading.
Loading..
Loading...
Done!
```

注意每次都在同一行更新，而不是打印新行。这就是进度条的基础。

### ProgressBar 类设计

我们的进度条需要显示：
- 可视化进度条（如 `[=====>     ]`）
- 百分比
- 已拷贝/总大小
- 当前速度
- 预估剩余时间（ETA）

```cpp
class ProgressBar {
public:
  explicit ProgressBar(int width = 20) : bar_width_(width) {}

  void update(std::uintmax_t copied, std::uintmax_t total,
              double speed_bytes_per_s) const;

private:
  int bar_width_;
};
```

`update()` 方法接收当前已拷贝字节数、总字节数和当前速度，然后更新进度显示。

### 绘制进度条

进度条是字符串 `[=====>     ]` 这样的形式，我们需要计算填充部分和空白部分：

```cpp
void ProgressBar::update(std::uintmax_t copied, std::uintmax_t total,
                         double speed_bytes_per_s) const {
  double fraction = (total == 0) ? 1.0 : static_cast<double>(copied) / total;
  int filled = static_cast<int>(fraction * bar_width_);

  // 绘制条形
  std::cout << "[";
  for (int i = 0; i < filled; ++i)
    std::cout << "=";
  if (filled < bar_width_)
    std::cout << ">";  // 当前进度指示符
  for (int i = filled + 1; i < bar_width_; ++i)
    std::cout << " ";
  std::cout << "] ";
```

首先计算已填充的比例 `fraction`，然后根据 `bar_width_` 计算填充的字符数。`>` 符号表示当前的进度位置。

### 计算百分比和大小

```cpp
  // 百分比和大小
  double percent = fraction * 100.0;
  double copied_mb = static_cast<double>(copied) / (1024.0 * 1024.0);
  double total_mb = static_cast<double>(total) / (1024.0 * 1024.0);
```

百分比就是比例乘以 100。大小我们用 MB 作为单位显示，因为对于大多数文件来说这是最直观的。如果文件太小（< 1MB），会显示小数如 `0.5MB`；如果太大，就用更大的数字如 `1024.5MB`。

### 计算和格式化 ETA

ETA（Estimated Time of Arrival，预估剩余时间）的计算基于当前速度：

```cpp
  double eta_seconds = 0.0;
  if (speed_bytes_per_s > 1e-6 && copied < total)
    eta_seconds = static_cast<double>(total - copied) / speed_bytes_per_s;
```

剩余字节数除以速度就是剩余秒数。注意我们检查 `speed > 1e-6`，避免除以接近零的数。同时检查 `copied < total`，拷贝完成后不需要 ETA。

格式化 ETA 时，我们根据时长选择合适的单位：

```cpp
  std::cout << std::fixed << std::setprecision(1)
            << percent << "% | "
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
```

- 超过 1 小时：显示 `Xh Ym`
- 超过 1 分钟：显示 `Xm Ys`
- 小于 1 分钟：显示 `Xs`

这里 `std::fixed` 和 `std::setprecision(1)` 确保浮点数以固定小数位数显示。

### 完成更新

最后是回车和刷新：

```cpp
  std::cout << '\r' << std::flush;
}
```

注意这里没有用 `std::endl`，因为它会换行。我们只想回到行首，不需要换行。`std::flush` 确保输出立即显示，否则可能因为缓冲导致进度条不更新。

### 计算速度

速度的计算在拷贝循环中进行：

```cpp
auto t_start = std::chrono::steady_clock::now();
auto last_report = t_start;

while (in) {
  // ... 读写操作 ...

  // 每 0.1 秒更新一次进度
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
```

这里我们用 `std::chrono::steady_clock` 来测量时间，它适合测量时间间隔，不受系统时间调整影响。

我们不是每次拷贝一块就更新进度，而是每 0.1 秒更新一次。原因是：

1. 终端刷新开销不小，频繁更新会拖慢速度
2. 人眼也分辨不出太频繁的更新
3. 0.1 秒的更新频率已经很流畅了

注意最后的 `|| copied == total_size`，确保拷贝完成后至少更新一次进度条，显示最终状态。

### 完整的拷贝循环

整合进度条后，完整的拷贝循环是：

```cpp
std::vector<char> buffer(chunk_size_);
std::uintmax_t copied = 0;

auto t_start = std::chrono::steady_clock::now();
auto last_report = t_start;

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
  if (read_bytes <= 0) break;

  out.write(buffer.data(), read_bytes);
  if (!out) {
    std::cerr << "Write error while writing to: " << dst_path << "\n";
    return false;
  }

  copied += static_cast<std::uintmax_t>(read_bytes);

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
```

### 打印最终结果

拷贝完成后，我们打印最后一次进度并换行：

```cpp
// 确保数据刷新
out.flush();
out.close();
in.close();

// 最终进度行（完成）
auto t_end = std::chrono::steady_clock::now();
std::chrono::duration<double> total_elapsed = t_end - t_start;
double avg_speed =
    (total_elapsed.count() > 1e-9)
        ? (static_cast<double>(copied) / total_elapsed.count())
        : 0.0;
bar.update(copied, total_size, avg_speed);
std::cout << "\n";
```

注意我们用整个拷贝过程的平均速度作为最终速度，而不是瞬时速度。这样显示的 ETA 会是 0s（因为已完成）。

### 小结

到这里，进度条的核心功能就实现了。你可能注意到了一个设计细节：`ProgressBar` 的 `update()` 方法是 `const` 的，因为它不修改成员变量。所有状态都通过参数传入。这使得 `ProgressBar` 可以是无状态的，更简单更安全。

下一章我们会把所有代码整合起来，进行完整的测试和验证。

[下一章：完整流程与验证 →](03-complete.md)
