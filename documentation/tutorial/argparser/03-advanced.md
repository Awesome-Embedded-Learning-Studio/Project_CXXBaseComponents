# 03 - 高级特性

## 类型安全的 get() 函数

解析完参数后，我们需要一个方便的接口来获取参数值。理想情况下，我们希望用户可以这样写：

```cpp
int count = parser.get<int>("count");
std::string output = parser.get<std::string>("output");
double ratio = parser.get<double>("ratio");
```

这需要一个模板函数。但有个问题：我们存储的所有值都是字符串形式，怎么转换成用户想要的类型？

### 字符串到任意类型的转换

C++ 里最通用的字符串转换方式是流（stream）：`std::istringstream`。它支持 `>>` 操作符，可以把字符串转换成任何支持 `operator>>` 的类型：

```cpp
template <typename T>
[[nodiscard]] T ArgParser::get(const std::string &key) const {
  auto it = args_.find(key);
  if (it != args_.end() && !it->second.value.empty()) {
    T result;
    std::istringstream(it->second.value) >> result;
    return result;
  }

  // 如果在命名参数里找不到，试试位置参数
  auto pit = pos_values.find(key);
  if (pit != pos_values.end()) {
    T result;
    std::istringstream(pit->second) >> result;
    return result;
  }

  throw std::runtime_error("Argument not found: " + key);
}
```

这个函数首先在命名参数里查找，找不到就去位置参数里找，都找不到就抛异常。这里有个设计决策：如果参数存在但值为空（比如 flag 类型的参数），我们会跳过转换，继续查找位置参数。这是因为 flag 类型的参数应该用 `get_flag()` 来获取，而不是 `get()`。

### std::string 的特殊处理

但这个通用实现有个问题：对于 `std::string` 类型，用 `istringstream` 转换不是最优的。更糟糕的是，它无法正确处理带空格的字符串。比如：

```cpp
std::istringstream iss("hello world");
std::string str;
iss >> str;  // str 只会是 "hello"，"world" 被忽略了！
```

这是因为 `operator>>` 默认以空白字符作为分隔符。对于字符串类型，我们应该直接返回原值，而不是通过流转换。

这就是模板特化的用武之地。我们可以为 `std::string` 提供一个特化版本：

```cpp
template <>
[[nodiscard]] inline std::string ArgParser::get<std::string>(const std::string &key) const {
  auto it = args_.find(key);
  if (it != args_.end() && !it->second.value.empty()) {
    return it->second.value;
  }

  auto pit = pos_values.find(key);
  if (pit != pos_values.end()) {
    return std::string{pit->second};
  }

  throw std::runtime_error("Argument not found: " + key);
}
```

这里 `template <>` 表示这是一个特化版本，`inline` 是为了避免重复定义错误（因为函数定义在头文件里）。对于字符串类型，我们直接返回原值，不需要任何转换。

注意这里有个细节：`return std::string{pit->second}`。因为 `pit->second` 是 `std::string` 类型，而 `pos_values` 的 value 类型是 `std::string`，所以这里直接构造一个新的 `std::string` 返回。虽然有点多余，但保持一致性也没什么坏处。

### get_flag() 函数

对于布尔类型的 flag，我们提供专门的 `get_flag()` 函数：

```cpp
bool ArgParser::get_flag(const std::string &key) const {
  auto it = args_.find(key);
  if (it != args_.end()) {
    return it->second.is_set;
  }
  return false;
}
```

这个函数只检查 `is_set` 标志，不关心 `value` 字段。如果参数不存在，返回 `false` 而不是抛异常，这是一个合理的设计——用户没传 flag 就当它是 false。

### 自动生成帮助信息

最后一个重要功能是 `--help` 自动生成帮助信息。这个功能的核心是 `print_help()` 函数，它根据注册的参数自动生成格式化的帮助文本。

```cpp
void ArgParser::print_help(std::string_view program_name) const {
  std::cout << "Usage: " << program_name;

  // 打印位置参数占位符
  for (const auto &pd : pos_args) {
    std::cout << " <" << pd.name << ">";
  }
  std::cout << " [options]\n\n";

  // 打印位置参数列表
  if (!pos_args.empty()) {
    std::cout << "Positional arguments:\n";
    for (const auto &pd : pos_args) {
      std::string label = "<" + pd.name + ">";
      std::cout << "  " << std::left << std::setw(24) << label << pd.help;
      if (!pd.required) {
        std::cout << " [optional]";
      }
      std::cout << "\n";
    }
    std::cout << "\n";
  }

  // 打印命名参数列表
  std::cout << "Options:\n";
  for (const auto &[key, arg] : args_) {
    std::string flags = arg.short_name + ", " + arg.long_name;
    if (!arg.is_flag) {
      flags += " <value>";
    }
    std::cout << "  " << std::left << std::setw(24) << flags << arg.help;
    if (arg.required) {
      std::cout << " [required]";
    }
    std::cout << "\n";
  }
  // 帮助选项
  std::cout << "  " << std::left << std::setw(24) << "-h, --help"
            << "Show this help message\n";
}
```

这个函数没什么复杂逻辑，主要是格式化输出。我们用 `std::setw(24)` 来对齐帮助信息，让输出看起来整齐一些。`std::left` 表示左对齐，这样长一点的文字也不会错位。

帮助信息的格式大致是这样的：

```
Usage: ./program <input> <output> [options]

Positional arguments:
  <input>              Input file path
  <output>             Output file path

Options:
  -v, --verbose        Enable verbose output
  -c, --count <value>  Number of iterations [required]
  -h, --help           Show this help message
```

### 使用示例

现在我们的 ArgParser 已经完成了，让我们看一个完整的使用示例：

```cpp
#include "ArgParser.h"
#include <iostream>

int main(int argc, char *argv[]) {
  cc::ArgParser parser;

  // 添加参数
  parser.add_argument("-i", "--input", "Input file path", true);
  parser.add_argument("-o", "--output", "Output file path", true);
  parser.add_argument("-n", "--count", "Iteration count");
  parser.add_flag("-v", "--verbose", "Enable verbose output");
  parser.add_positional("mode", "Operation mode", false);

  // 解析参数
  try {
    parser.parse(argc, argv);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  // 获取参数值
  std::string input = parser.get<std::string>("input");
  std::string output = parser.get<std::string>("output");
  int count = parser.get<int>("count");
  bool verbose = parser.get_flag("verbose");

  std::cout << "Input: " << input << "\n";
  std::cout << "Output: " << output << "\n";
  std::cout << "Count: " << count << "\n";
  std::cout << "Verbose: " << (verbose ? "yes" : "no") << "\n";

  return 0;
}
```

运行示例：

```bash
$ ./demo -i in.txt -o out.txt -n 10 -v
Input: in.txt
Output: out.txt
Count: 10
Verbose: yes

$ ./demo --help
Usage: ./demo <mode> [options]

Positional arguments:
  <mode>               Operation mode [optional]

Options:
  -i, --input <value>      Input file path [required]
  -o, --output <value>     Output file path [required]
  -n, --count <value>      Iteration count
  -v, --verbose            Enable verbose output
  -h, --help               Show this help message
```

### 小结

到这里，我们的 ArgParser 就完整了。虽然代码量不大，但它涵盖了命令行参数解析的所有核心功能：

- 短参数和长参数
- 值参数和布尔开关
- 位置参数
- 必填检查
- 自动生成帮助信息
- 类型安全的参数获取

更重要的是，我们在这个过程中学到了很多现代 C++ 的实践：`std::string_view` 避免拷贝、模板特化处理特殊情况、`std::unordered_map` 高效查找、异常处理错误情况。

这个 ArgParser 虽然简单，但它是一个完整可用的工具。你可以在自己的项目中使用它，也可以作为学习材料，理解命令行参数解析的原理。当你真正理解了这些底层机制，以后使用任何第三方库都会更加得心应手。

---

## 完结撒花

恭喜你，现在你已经拥有了一个完全自己掌控的命令行参数解析器。虽然它可能不如某些成熟的库功能丰富，但它简洁、清晰、完全可定制。更重要的是，你理解了它的每一行代码，这是使用任何第三方库都无法替代的。

去写点命令行工具吧，你会发现有了这个 ArgParser，一切都变得顺手多了。

[← 返回教程首页](README.md)
