# DirScanner 项目讲解稿

## 目录
1. [项目概述](#项目概述)
2. [项目结构](#项目结构)
3. [核心代码讲解](#核心代码讲解)
4. [C++技术要点](#c技术要点)
5. [构建系统](#构建系统)
6. [使用示例](#使用示例)

---

## 项目概述

**DirScanner** 是一个现代化的 C++ 目录扫描工具，具有以下特点：

- 递归扫描指定目录
- 统计文件和目录数量
- 计算总文件大小
- 按文件扩展名分类统计
- 找出最大的10个文件
- 支持自定义扫描深度
- 支持表格和JSON两种输出格式

### 技术栈
- **C++17**：利用 `std::filesystem` 文件系统库
- **CMake**：跨平台构建系统
- **第三方库**：ArgParser（命令行参数解析）

---

## 项目结构

```
DirScanner/
├── src/
│   ├── dir_scan.h       # DirScanner类声明
│   ├── dir_scan.cpp     # DirScanner类实现
│   └── main.cpp         # 主程序入口
├── test/
│   └── test.cpp         # 简单单元测试
├── third_party/
│   ├── ArgParser.h      # 第三方参数解析库
│   └── ArgParser.cpp
├── CMakeLists.txt       # CMake构建配置
├── .gitignore
└── TUTORIAL.md          # 本文档
```

**设计模式**：采用**库+应用分离**的架构
- `libDirScanner.a`：核心扫描逻辑，可被其他程序复用
- `dirscanner`：可执行程序，展示库的使用方式

---

## 核心代码讲解

### 一、数据结构设计 (dir_scan.h)

#### 1.1 FileInfo 结构体
```cpp
struct FileInfo {
    fs::path path;      // 文件完整路径
    uintmax_t size;     // 文件大小（字节）
};
```
- `fs::path`：C++17 filesystem库提供的路径类型，自动处理跨平台路径分隔符
- `uintmax_t`：无符号整数最大类型，确保能存储超大文件大小

#### 1.2 ScanResult 结构体
```cpp
struct ScanResult {
    size_t file_count = 0;                           // 文件总数
    size_t dir_count = 0;                            // 目录总数
    uintmax_t total_size = 0;                        // 总大小
    std::unordered_map<std::string, size_t> ext_count; // 扩展名统计
    std::vector<FileInfo> largest_files;             // 最大文件列表
};
```
**设计要点**：
- 使用 `std::unordered_map` 而非 `std::map`：O(1) 查找，适合扩展名统计
- 成员变量默认初始化（C++11特性）：`= 0` 确保数值从0开始计数

#### 1.3 DirScanner 类
```cpp
class DirScanner {
public:
    DirScanner(size_t max_depth = SIZE_MAX);

    ScanResult scan(const fs::path& root);

private:
    void scan_impl(const fs::path& path, size_t depth);
    void add_file(const fs::directory_entry& entry);

private:
    size_t max_depth_;           // 最大扫描深度
    ScanResult result_;          // 扫描结果
    std::vector<FileInfo> all_files_;  // 所有文件列表（用于排序）
};
```
**设计模式**：
- **Pimpl 思想简化版**：将实现细节 `scan_impl` 和 `add_file` 设为私有
- **单一职责原则**：`scan` 负责对外接口，`scan_impl` 负责递归实现，`add_file` 负责单文件处理

---

### 二、核心实现 (dir_scan.cpp)

#### 2.1 构造函数
```cpp
DirScanner::DirScanner(size_t max_depth)
    : max_depth_(max_depth) {}
```
- 使用**成员初始化列表**，比在构造函数体内赋值更高效
- 默认参数 `SIZE_MAX` 表示无深度限制

#### 2.2 scan() 方法 - 主入口
```cpp
ScanResult DirScanner::scan(const fs::path& root) {
    result_ = {};           // 重置结果（聚合初始化）
    all_files_.clear();     // 清空文件列表

    try {
        // 1. 检查路径存在性
        if (!fs::exists(root)) {
            throw std::runtime_error("Path does not exist");
        }

        // 2. 开始递归扫描
        scan_impl(root, 0);

        // 3. Top-K 算法：使用 priority_queue 实现堆选择
        constexpr size_t TOP_K = 10;
        size_t top_n = std::min<size_t>(TOP_K, all_files_.size());

        if (top_n > 0) {
            // 小顶堆：堆顶是当前第 k 大的元素
            auto cmp = [](const FileInfo& a, const FileInfo& b) {
                return a.size > b.size;
            };
            std::priority_queue<FileInfo, std::vector<FileInfo>, decltype(cmp)> min_heap(cmp);

            for (const auto& file : all_files_) {
                if (min_heap.size() < top_n) {
                    min_heap.push(file);           // 堆未满，直接插入
                } else if (file.size > min_heap.top().size) {
                    min_heap.pop();                 // 移除最小的
                    min_heap.push(file);            // 插入更大的
                }
            }

            // 转移结果并反转（小顶堆弹出顺序是升序）
            result_.largest_files.reserve(top_n);
            while (!min_heap.empty()) {
                result_.largest_files.push_back(min_heap.top());
                min_heap.pop();
            }
            std::reverse(result_.largest_files.begin(),
                         result_.largest_files.end());
        }

    } catch (const std::exception& e) {
        std::cerr << "Scan error: " << e.what() << std::endl;
    }

    return result_;
}
```

**技术要点解析**：

| 技术 | 说明 |
|------|------|
| `result_ = {}` | C++11聚合初始化，所有成员归零 |
| `fs::exists()` | 检查路径是否存在 |
| `std::priority_queue` | 适配器容器，底层默认使用 `vector` + `make_heap` 实现 |
| `constexpr` | 编译期常量，提升性能 |
| `decltype(cmp)` | Lambda 类型推导，用于 priority_queue 模板参数 |
| `reserve()` | 预分配内存，避免 vector 动态扩容 |

**Top-K 算法核心思想（堆选择）**：

```
问题：从 n 个元素中找出最大的 k 个
解法：维护一个大小为 k 的小顶堆

┌─────────────────────────────────────────────┐
│  小顶堆特性：堆顶是最小的元素                 │
│                                             │
│        [50]          ← 堆顶（当前第3大）     │
│       /    \                                │
│    [80]    [100]                            │
│                                             │
│  遇到新文件 120 > 50：                       │
│  1. 弹出 50                                  │
│  2. 插入 120                                 │
│  3. 堆变为 [80, 120, 100]                   │
└─────────────────────────────────────────────┘
```

**复杂度对比**：

| 算法 | 时间复杂度 | 空间复杂度 | 说明 |
|------|-----------|-----------|------|
| `std::sort` | O(n log n) | O(1) | 全排序，适合需要全部有序 |
| `std::partial_sort` | O(n log k) | O(1) | STL 提供的 top-k 算法 |
| **priority_queue** | **O(n log k)** | **O(k)** | **手动实现，更灵活** |

**为什么用小顶堆而非大顶堆？**
- 小顶堆：堆顶是最小的，遇到更大的元素直接替换堆顶 O(log k)
- 大顶堆：需要遍历整个堆才能找到最小元素 O(k)
- 对于"最大 k 个"，小顶堆是正确选择

#### 2.3 scan_impl() 方法 - 递归扫描
```cpp
void DirScanner::scan_impl(const fs::path& path, size_t depth) {
    // 深度检查
    if (depth > max_depth_) return;

    try {
        // directory_iterator：遍历目录内容
        for (const auto& entry : fs::directory_iterator(path)) {
            try {
                if (entry.is_directory()) {
                    result_.dir_count++;
                    scan_impl(entry.path(), depth + 1);  // 递归
                } else if (entry.is_regular_file()) {
                    add_file(entry);
                }
            } catch (const fs::filesystem_error& e) {
                // 单个条目处理失败不影响整体扫描
                std::cerr << "Permission denied or error: "
                          << entry.path() << "\n";
            }
        }
    } catch (const fs::filesystem_error& e) {
        // 无法访问整个目录
        std::cerr << "Cannot access: " << path << "\n";
    }
}
```

**设计亮点**：
1. **深度优先遍历**：遇到目录立即递归
2. **双层异常处理**：外层处理目录访问失败，内层处理单个文件失败
3. **健壮性优先**：权限不足时打印警告，继续扫描其他文件
4. **is_regular_file()**：跳过符号链接、设备文件等特殊文件

#### 2.4 add_file() 方法 - 文件处理
```cpp
void DirScanner::add_file(const fs::directory_entry& entry) {
    // 获取文件大小
    uintmax_t size = 0;
    try {
        size = entry.file_size();
    } catch (...) {
        size = 0;  // 获取失败时默认为0
    }

    // 更新统计信息
    result_.file_count++;
    result_.total_size += size;

    // 获取扩展名
    std::string ext = entry.path().extension().string();
    if (ext.empty()) ext = "[no_ext]";

    result_.ext_count[ext]++;

    // 保存到列表供后续排序
    all_files_.push_back({entry.path(), size});
}
```

**细节要点**：
- `extension()`：返回包括点号的扩展名，如 `.cpp`
- `"[no_ext]"`：为无扩展名文件提供默认标签
- `push_back({})`：使用初始化列表构造FileInfo
- 捕获所有异常 `catch (...)`：确保单个文件错误不影响整体扫描

---

### 三、主程序 (main.cpp)

#### 3.1 输出格式化函数

**表格格式输出**：
```cpp
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
```
- `std::setw(10)`：设置输出宽度，实现列对齐
- **结构化绑定** `const auto &[ext, count]`：C++17特性，解pair/map元素

**JSON格式输出**：
```cpp
void print_json(const ScanResult &r) {
  std::cout << "{\n";
  std::cout << "  \"file_count\": " << r.file_count << ",\n";
  // ... 更多字段

  std::cout << "  \"extensions\": {\n";
  for (auto it = r.ext_count.begin(); it != r.ext_count.end(); ++it) {
    std::cout << "    \"" << it->first << "\": " << it->second;
    if (std::next(it) != r.ext_count.end())
      std::cout << ",";  // 不是最后一个元素则加逗号
    std::cout << "\n";
  }
  // ...
}
```
- 手动构建JSON，避免依赖JSON库
- `std::next(it)`：迭代器算术，判断是否为最后一个元素

#### 3.2 主函数
```cpp
int main(int argc, char *argv[]) {
  cc::ArgParser parser;

  // 添加命令行参数
  parser.add_argument("-p", "--path", "Directory path to scan", false);
  parser.add_argument("-d", "--depth", "Max scan depth (0 = unlimited)", false);
  parser.add_flag("-j", "--json", "Output in JSON format");

  parser.parse(argc, argv);

  // 获取路径参数，默认为当前目录
  std::string path = ".";
  try {
    std::string p = parser.get<std::string>("path");
    if (!p.empty())
      path = p;
  } catch (...) {
  }

  // 获取深度参数，默认为无限制
  size_t depth = SIZE_MAX;
  try {
    depth = parser.get<size_t>("depth");
  } catch (...) {
  }

  bool json = parser.get_flag("json");

  // 执行扫描
  DirScanner scanner(depth);
  auto result = scanner.scan(path);

  // 输出结果
  if (json)
    print_json(result);
  else
    print_table(result);

  return 0;
}
```

**使用技巧**：
- 使用 `try-catch` 处理可选参数，未提供时使用默认值
- `auto` 类型推导：简化代码，增强可读性
- `SIZE_MAX`：`<cstddef>` 中定义，表示 `size_t` 的最大值

---

## C++技术要点

### 1. std::filesystem (C++17)

| 功能 | 代码示例 | 说明 |
|------|----------|------|
| 路径表示 | `fs::path p("/home/user/docs");` | 跨平台路径处理 |
| 检查存在 | `fs::exists(p)` | 判断路径是否存在 |
| 遍历目录 | `fs::directory_iterator(p)` | 迭代器模式遍历 |
| 文件大小 | `entry.file_size()` | 返回 `uintmax_t` |
| 扩展名 | `p.extension()` | 返回 `.txt` 等 |
| 判断类型 | `entry.is_directory()` / `is_regular_file()` | 区分文件类型 |

### 2. 现代 C++ 特性

```cpp
// 1. auto 类型推导
auto result = scanner.scan(path);

// 2. 范围 for 循环
for (const auto& entry : fs::directory_iterator(path)) { }

// 3. Lambda 表达式
[](const FileInfo& a, const FileInfo& b) { return a.size > b.size; }

// 4. 结构化绑定 (C++17)
for (const auto &[ext, count] : r.ext_count) { }

// 5. 成员初始化
size_t file_count = 0;

// 6. nullptr（虽然本例未使用，但推荐替代NULL）
```

### 3. 异常安全

```cpp
// 多层次异常处理策略
try {
    // 核心逻辑
} catch (const fs::filesystem_error& e) {
    // 特定异常：文件系统错误
} catch (const std::exception& e) {
    // 标准异常
} catch (...) {
    // 捕获所有异常
}
```

---

## 构建系统

### CMakeLists.txt 详解

```cmake
# 1. 项目声明
cmake_minimum_required(VERSION 3.15)
project(DirScanner VERSION 1.0.0 LANGUAGES CXX)

# 2. C++标准设置
set(CMAKE_CXX_STANDARD 17)          # 使用C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 必须支持C++17
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # 生成compile_commands.json供IDE使用

# 3. 头文件搜索路径
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
)

# 4. 第三方库（静态库）
add_library(ArgParser STATIC
    third_party/Parser.cpp
)
target_include_directories(ArgParser PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party
)

# 5. 核心库（静态库）
add_library(DirScanner STATIC
    src/dir_scan.cpp
)
target_include_directories(DirScanner PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# 6. 可执行文件
add_executable(dirscanner
    src/main.cpp
)
target_link_libraries(dirscanner
    PRIVATE
        DirScanner
        ArgParser
)

# 7. 可选的测试目标
if(BUILD_TESTING)
    add_executable(dirscanner_test
        test/test.cpp
    )
    target_link_libraries(dirscanner_test PRIVATE DirScanner)
endif()
```

### 构建步骤

```bash
# 1. 创建构建目录
mkdir build && cd build

# 2. 配置项目
cmake ..

# 3. 编译
cmake --build .

# 4. 运行
./dirscanner -p /path/to/scan -d 2

# 5. 构建测试（如果需要）
cmake .. -DBUILD_TESTING=ON
cmake --build .
./dirscanner_test
```

---

## 使用示例

### 基本用法

```bash
# 扫描当前目录，表格输出
./dirscanner

# 扫描指定目录
./dirscanner -p /home/user/projects

# 限制扫描深度为2层
./dirscanner -p /home/user -d 2

# JSON格式输出
./dirscanner -p /home/user -j

# 组合使用
./dirscanner --path /etc --depth 1 --json
```

### 输出示例

**表格格式**：
```
==== Scan Summary ====
Files: 42
Dirs : 8
Size : 15360 bytes

-- Extension Stats --
    .cpp : 12
         .h : 8
    .txt : 5
[no_ext] : 3
         .md : 2

-- Top 10 Largest Files --
8192    /home/user/project/src/main.cpp
4096    /home/user/project/include/header.h
1024    /home/user/project/README.md
...
```

**JSON格式**：
```json
{
  "file_count": 42,
  "dir_count": 8,
  "total_size": 15360,
  "extensions": {
    ".cpp": 12,
    ".h": 8,
    ".txt": 5,
    "[no_ext]": 3,
    ".md": 2
  },
  "largest_files": [
    { "path": "/home/user/project/src/main.cpp", "size": 8192 },
    { "path": "/home/user/project/include/header.h", "size": 4096 }
  ]
}
```

---

## 扩展建议

1. **功能扩展**
   - 添加文件类型过滤（只扫描特定扩展名）
   - 支持正则表达式匹配文件名
   - 添加并行扫描加速大目录处理
   - 支持输出到文件而非stdout

2. **性能优化**
   - 使用 `std::filesystem::recursive_directory_iterator` 简化代码
   - 添加进度回调函数
   - 实现增量扫描（只检测变化）

3. **代码质量**
   - 添加单元测试框架（Google Test）
   - 添加日志系统代替 std::cerr
   - 使用智能指针管理资源

---

## 总结

DirScanner 项目展示了现代 C++ 的最佳实践：

1. **清晰的代码组织**：头文件与实现分离，库与应用分离
2. **RAII 思想**：使用标准库容器自动管理内存
3. **异常安全**：多层次异常处理保证程序健壮性
4. **STL 算法**：使用 `std::sort` 等标准算法
5. **C++17 特性**：`std::filesystem`、结构化绑定等
6. **跨平台构建**：CMake 实现平台无关编译

这个项目虽小，但五脏俱全，是学习现代 C++ 的优秀范例。
