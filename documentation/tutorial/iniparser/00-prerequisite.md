# 00 - 前置知识

## 磨刀不误砍柴工

在开始写 INI 解析器之前，我们需要先掌握几个现代 C++ 的重要工具。这些不只是编写解析器的基础，更是日常 C++ 开发中的利器。如果你已经熟悉这些，可以快速扫过；如果不太熟悉，建议仔细阅读，后面的内容会大量使用这些特性。

### std::string_view：零拷贝的字符串视图

`std::string_view` 是 C++17 引入的一个非常有用的类型。它本质上是一个"窗口"——不拥有字符串数据，只是指向一个已有的字符串片段。

#### 为什么需要 string_view

假设你有一个函数，只需要读取字符串内容而不需要修改它：

```cpp
// 旧方式：按 const std::string& 传递
void print_length(const std::string& str) {
    std::cout << str.length() << "\n";
}

// 调用
std::string s = "hello";
print_length(s);           // OK
print_length("world");     // 隐式构造 string，有拷贝开销！
```

问题在于，当你传递字符串字面量时，编译器需要构造一个临时的 `std::string` 对象。对于短字符串这还好，但对于频繁调用的函数或长字符串，这个开销就不可忽视了。

使用 `std::string_view`：

```cpp
// 新方式：按 std::string_view 传递
void print_length(std::string_view sv) {
    std::cout << sv.length() << "\n";
}

// 调用
std::string s = "hello";
print_length(s);           // OK，零拷贝
print_length("world");     // OK，零拷贝
print_length(s.substr(1)); // OK，零拷贝（substr 返回 string）
```

`std::string_view` 可以从 `std::string`、C 风格字符串、字符串字面量零拷贝构造。

#### string_view 的陷阱

但 `std::string_view` 有一个致命的陷阱：它不拥有数据。如果原始字符串被销毁，`string_view` 就会变成悬垂指针：

```cpp
std::string_view get_view() {
    std::string s = "hello";
    return s;  // 错误！s 被销毁，返回的 view 指向已释放的内存
}

std::string_view sv = get_view();  // 未定义行为！
std::cout << sv;  // 可能崩溃，也可能打印乱码
```

记住这个原则：**`string_view` 只是一个临时视图，不要把它存储起来长期使用**。如果你需要长期保存，就转换成 `std::string`。

#### 常用操作

```cpp
std::string str = "hello world";
std::string_view sv = str;

// 长度
std::cout << sv.length();   // 11
std::cout << sv.size();     // 11

// 访问字符
std::cout << sv[0];         // 'h'
std::cout << sv.front();    // 'h'
std::cout << sv.back();     // 'd'

// 移除前后字符
sv.remove_prefix(2);        // 变成 "llo world"
sv.remove_suffix(2);        // 变成 "llo worl"

// 子串
auto sub = sv.substr(0, 3); // 返回 string_view，指向 "llo"

// 比较
if (sv == "llo worl") { ... }
```

注意 `remove_prefix()` 和 `remove_suffix()` 是原地修改，不拷贝数据。

### std::optional：可能不存在的值

在处理配置文件时，我们经常遇到"某个值可能存在也可能不存在"的情况。传统的做法是用特殊值表示"不存在"（比如空字符串、-1、nullptr），但这容易产生歧义。

C++17 引入了 `std::optional<T>`，明确表达"可能有值也可能没有"的语义。

#### 基本用法

```cpp
#include <optional>

std::optional<int> find_value(const std::string& key) {
    if (key == "secret") {
        return 42;  // 有值
    }
    return std::nullopt;  // 没有值
}

// 使用
auto result = find_value("secret");
if (result.has_value()) {
    std::cout << "Found: " << result.value() << "\n";
} else {
    std::cout << "Not found\n";
}

// 更简洁的方式
if (result) {
    std::cout << "Found: " << *result << "\n";
}
```

#### 访问值的多种方式

```cpp
std::optional<int> opt = 42;

// value()：如果有值返回，否则抛出 std::bad_optional_access
int x = opt.value();

// value_or()：如果有值返回，否则返回默认值
int y = opt.value_or(0);  // 如果 opt 没值，返回 0

// operator* 和 operator->：解引用，要求必须有值
int z = *opt;

// 修改值
opt = 100;
opt.emplace(200);  // 就地构造
opt.reset();       // 清空值
```

#### 在配置文件中的应用

对于 INI 解析器，`std::optional` 非常适合表示"某个 key 可能存在"：

```cpp
class IniParser {
public:
    // 如果 key 不存在，返回空的 optional
    std::optional<std::string> get_string(const std::string& section,
                                          const std::string& key) const;

    std::optional<int> get_int(const std::string& section,
                               const std::string& key) const;
};

// 使用
auto port = parser.get_int("server", "port");
if (port) {
    start_server(*port);
} else {
    start_server(8080);  // 默认端口
}
```

### split 和 trim：字符串处理的两个好基友

在解析文本时，两个最常用的操作是：按分隔符切分字符串（split），以及去除字符串两端的空白字符（trim）。

#### trim 的实现

trim 就是去掉字符串两端的空格、制表符、换行符等空白字符：

```cpp
#include <string>
#include <cctype>
#include <string_view>

// 左 trim
std::string_view ltrim(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    return sv;
}

// 右 trim
std::string_view rtrim(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return sv;
}

// 两边都 trim
std::string_view trim(std::string_view sv) {
    return ltrim(rtrim(sv));
}
```

这里我们用 `std::string_view` 实现零拷贝的 trim。但注意返回的 `string_view` 只是原字符串的视图，如果原字符串被销毁，返回值就失效了。如果需要长期保存，转换成 `std::string`：

```cpp
std::string s = trim(sv);  // 拷贝到 string
```

注意 `static_cast<unsigned char>` 是必要的，因为 `std::isspace()` 的参数必须是 `unsigned char` 或 `EOF`。直接传 `char` 可能导致符号扩展问题（char 可能是 signed 的）。

#### split 的实现

split 按分隔符把字符串切分成多个部分：

```cpp
#include <vector>
#include <string_view>

std::vector<std::string> split(std::string_view sv, char delim) {
    std::vector<std::string> result;
    std::string_view::size_type start = 0;
    std::string_view::size_type pos = sv.find(delim);

    while (pos != std::string_view::npos) {
        result.emplace_back(sv.substr(start, pos - start));
        start = pos + 1;
        pos = sv.find(delim, start);
    }

    // 添加最后一个部分
    result.emplace_back(sv.substr(start));
    return result;
}
```

这个实现有几个注意点：

1. 使用 `std::string_view::find()` 而不是 `std::string::find()`，避免构造 string
2. 最后一个部分需要单独添加，因为它后面没有分隔符了
3. 返回 `std::vector<std::string>` 而不是 `vector<string_view>`，因为原始字符串可能被销毁

使用示例：

```cpp
auto parts = split("a,b,c", ',');
// parts = ["a", "b", "c"]

auto parts2 = split("x;y;z", ';');
// parts2 = ["x", "y", "z"]

// 空字符串也能处理
auto parts3 = split("", ',');
// parts3 = [""]

// 连续分隔符
auto parts4 = split("a,,b", ',');
// parts4 = ["a", "", "b"]
```

#### 在 INI 解析中的应用

有了这两个工具，解析 INI 文件就方便多了：

```cpp
// 解析 "section.key = value"
auto parts = split(line, '=');
if (parts.size() == 2) {
    auto key = trim(parts[0]);
    auto value = trim(parts[1]);
    // 处理 key 和 value
}

// 解析 "[section]"
if (line.front() == '[' && line.back() == ']') {
    auto section = trim(line.substr(1, line.length() - 2));
    // 处理 section
}
```

### 小结

这三个工具——`std::string_view`、`std::optional`、split/trim——是编写高效文本处理代码的基础。`string_view` 让我们避免不必要的拷贝，`optional` 明确表达"可能没有值"的语义，split/trim 是文本处理的瑞士军刀。

下一章我们开始设计 INI 解析器，这些工具都会派上用场。

[下一章：需求分析与设计 →](01-design.md)
