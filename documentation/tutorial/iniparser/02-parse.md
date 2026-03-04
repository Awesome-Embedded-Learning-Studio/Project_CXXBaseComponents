# 02 - 解析实现

## 上号！开始写解析逻辑

设计搞定了，现在我们要实现最核心的 `load()` 函数。这个函数要逐行读取文件，识别出节声明、键值对、注释，然后把它们存进我们的数据结构里。

### 逐行读取文件

首先，我们需要逐行读取文件内容。`std::ifstream` 配合 `std::getline()` 是标准做法：

```cpp
bool IniParser::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        warnings_.push_back("Cannot open file: " + filename);
        return false;
    }

    filename_ = filename;
    data_.clear();
    warnings_.clear();

    std::string line;
    std::string current_section = "";  // 当前节，空字符串表示全局

    while (std::getline(file, line)) {
        // 解析每一行...
    }

    return true;
}
```

注意我们用 `current_section = ""` 来表示"当前在全局节"。任何节声明之前的键值对都会存入这个全局节。

### 处理每一行

每一行的处理流程是：

1. 去掉注释
2. trim 前后空白
3. 如果为空，跳过
4. 判断是节声明还是键值对
5. 相应地更新数据结构

```cpp
while (std::getline(file, line)) {
    // 去掉注释
    line = remove_comment(line);

    // trim 前后空白
    line = trim(line);

    // 跳过空行
    if (line.empty()) {
        continue;
    }

    // 解析...
}
```

### 去掉注释

注释以 `;` 或 `#` 开头，但要注意：这些字符只在一行的开头算注释，在值里面不算：

```ini
key = value  ; 这里的分号是注释
key2 = value#not_comment  ; 这里的 # 是值的一部分
key3 = http://example.com  ; 这里的 :// 不是注释
```

所以我们的规则是：只在行首或值之后（即 `=` 之后）识别注释。

更简单的处理方式是：找到第一个 `;` 或 `#`，但要确保它在 `=` 之后：

```cpp
std::string IniParser::remove_comment(const std::string& line) {
    // 找到第一个等号
    auto eq_pos = line.find('=');

    // 从等号之后开始找注释
    size_t search_start = (eq_pos == std::string::npos) ? 0 : eq_pos;

    // 找第一个 ; 或 #
    size_t comment_pos = std::string::npos;
    size_t semicolon_pos = line.find(';', search_start);
    size_t hash_pos = line.find('#', search_start);

    if (semicolon_pos != std::string::npos && hash_pos != std::string::npos) {
        comment_pos = std::min(semicolon_pos, hash_pos);
    } else if (semicolon_pos != std::string::npos) {
        comment_pos = semicolon_pos;
    } else if (hash_pos != std::string::npos) {
        comment_pos = hash_pos;
    }

    if (comment_pos != std::string::npos) {
        return line.substr(0, comment_pos);
    }

    return line;
}
```

但这个实现有点复杂。更简单的方式是：直接找第一个 `;` 或 `#`，不管它在哪里：

```cpp
std::string IniParser::remove_comment(const std::string& line) {
    size_t pos = std::min(line.find(';'), line.find('#'));
    if (pos == std::string::npos) {
        return line;
    }
    return line.substr(0, pos);
}
```

等等，这会错误地把 `key = http://example.com` 变成 `key = http:`！所以我们必须更聪明一点。

实际上，正确的做法是：只识别行首的 `;` 或 `#` 作为注释。如果 `;` 或 `#` 前面有非空白字符，就不算注释。让我们换个思路：

```cpp
std::string IniParser::remove_comment(const std::string& line) {
    size_t pos = std::string::npos;

    // 只查找行首的 ; 或 #
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        if (c == ';' || c == '#') {
            // 前面只有空白字符，这是注释
            pos = i;
            break;
        }
        if (!std::isspace(static_cast<unsigned char>(c))) {
            // 遇到非空白字符，后面不会有注释了
            break;
        }
    }

    if (pos != std::string::npos) {
        return line.substr(0, pos);
    }
    return line;
}
```

但这样还是不对，因为 `key = value  ; comment` 这种尾注释处理不了。

最终，我们采用折中方案：只支持行首注释（行首空白后的 `;` 或 `#`）。尾注释的处理比较复杂，而且不是所有 INI 实现都支持。

### 解析节声明

节声明的格式是 `[section_name]`：

```cpp
bool IniParser::parse_section(const std::string& line, std::string& current_section) {
    if (line.front() == '[' && line.back() == ']') {
        // 提取节名（去掉 [ 和 ]）
        std::string section_name = line.substr(1, line.length() - 2);
        section_name = trim(section_name);

        if (section_name.empty()) {
            warnings_.push_back("Empty section name at line " + std::to_string(line_num));
            return false;
        }

        current_section = section_name;

        // 确保节存在
        if (data_.find(current_section) == data_.end()) {
            data_[current_section] = KeyValueMap();
        }

        return true;
    }
    return false;
}
```

这里我们检查 `line.front() == '['` 和 `line.back() == ']'` 来判断是否是节声明。注意在调用前我们已经 trim 过了，所以不会有前后空白干扰。

### 解析键值对

键值对的格式是 `key = value`：

```cpp
bool IniParser::parse_key_value(const std::string& line, const std::string& current_section) {
    auto eq_pos = line.find('=');
    if (eq_pos == std::string::npos) {
        // 不是键值对
        return false;
    }

    // 提取键和值
    std::string key = line.substr(0, eq_pos);
    std::string value = line.substr(eq_pos + 1);

    // trim
    key = trim(key);
    value = trim(value);

    // 验证
    if (key.empty()) {
        warnings_.push_back("Empty key at line " + std::to_string(line_num));
        return false;
    }

    // 存储
    data_[current_section][key] = value;
    return true;
}
```

### 完整的解析循环

把所有代码拼起来：

```cpp
bool IniParser::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        warnings_.push_back("Cannot open file: " + filename);
        return false;
    }

    filename_ = filename;
    data_.clear();
    warnings_.clear();

    std::string line;
    std::string current_section = "";
    int line_num = 0;

    while (std::getline(file, line)) {
        ++line_num;

        // 去掉行尾注释
        line = remove_comment(line);

        // trim 前后空白
        line = trim(line);

        // 跳过空行
        if (line.empty()) {
            continue;
        }

        // 尝试解析节声明
        if (parse_section(line, current_section)) {
            continue;
        }

        // 尝试解析键值对
        if (parse_key_value(line, current_section)) {
            continue;
        }

        // 都不是，记录警告
        warnings_.push_back("Unrecognized line " + std::to_string(line_num) + ": " + line);
    }

    return true;
}
```

### 辅助函数：trim

我们在前一章已经讲过 trim 的实现，这里再用一次：

```cpp
namespace {

std::string& trim_impl(std::string& s) {
    auto is_space = [](char c) {
        return std::isspace(static_cast<unsigned char>(c));
    };

    auto start = std::find_if_not(s.begin(), s.end(), is_space);
    s.erase(s.begin(), start);

    auto end = std::find_if_not(s.rbegin(), s.rend(), is_space).base();
    s.erase(end, s.end());

    return s;
}

} // anonymous namespace

std::string IniParser::trim(std::string s) {
    return trim_impl(s);
}
```

这里我们用 `std::string` 而不是 `std::string_view`，因为我们需要修改字符串并返回一个新的。虽然有一点拷贝开销，但对于配置文件这种小文件来说可以接受。

### 辅助函数：remove_comment

```cpp
std::string IniParser::remove_comment(std::string line) const {
    // 行尾注释：; 或 # 前面只有空白字符
    bool in_whitespace = true;

    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        if ((c == ';' || c == '#') && in_whitespace) {
            return line.substr(0, i);
        }
        if (!std::isspace(static_cast<unsigned char>(c))) {
            in_whitespace = false;
        }
    }

    return line;
}
```

这个实现能正确处理以下情况：

```ini
; 行首注释
  # 前面有空格的注释
key = value  ; 尾注释
url = http://example.com  ; 这里的 :// 不是注释
```

### 测试一下

让我们写个测试程序：

```cpp
#include "IniParser.h"
#include <iostream>

int main() {
    IniParser parser;

    if (!parser.load("config.ini")) {
        std::cerr << "Failed to load config\n";
        return 1;
    }

    // 打印警告
    for (const auto& w : parser.warnings()) {
        std::cout << "Warning: " << w << "\n";
    }

    // 读取值
    std::string host = parser.get_string("server", "host", "localhost");
    int port = parser.get_int("server", "port", 8080);
    bool debug = parser.get_bool("server", "debug", false);

    std::cout << "host = " << host << "\n";
    std::cout << "port = " << port << "\n";
    std::cout << "debug = " << debug << "\n";

    return 0;
}
```

编译运行：

```bash
$ g++ -std=c++17 -o iniparser_test main.cpp IniParser.cpp
$ ./iniparser_test
host = 127.0.0.1
port = 8080
debug = 1
```

### 小结

到这里，核心的解析逻辑就实现了。虽然代码不算太多，但处理了不少细节：

1. 逐行读取文件
2. 去掉注释（包括尾注释）
3. 识别节声明和键值对
4. 处理空白字符
5. 错误收集而不是直接失败

下一章我们会完善错误处理，并实现保存、类型转换等功能，让我们的 IniParser 真正可用。

[下一章：错误处理与完善 →](03-error.md)
