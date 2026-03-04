# 03 - 错误处理与完善

## 让解析器更健壮

上一章我们实现了核心的解析逻辑，但还有一些重要的功能没完成：保存文件、类型转换、错误处理等。这一章我们来补齐这些短板。

### 保存到文件

保存文件相对简单，就是把我们的数据结构写回 INI 格式：

```cpp
bool IniParser::save(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    // 先写全局节（空字符串表示的节）
    auto global_it = data_.find("");
    if (global_it != data_.end()) {
        for (const auto& [key, value] : global_it->second) {
            file << key << " = " << value << "\n";
        }
        file << "\n";
    }

    // 写其他节
    for (const auto& [section, kv_map] : data_) {
        if (section.empty()) continue;  // 跳过全局节，已经写过了

        file << "[" << section << "]\n";
        for (const auto& [key, value] : kv_map) {
            file << key << " = " << value << "\n";
        }
        file << "\n";
    }

    return file.good();
}
```

注意我们：

1. 先写全局节（如果有）
2. 然后按顺序写其他节
3. 每个节后加一个空行，提高可读性
4. 最后检查 `file.good()` 确保写入成功

### 类型转换函数

我们已经在前面的章节讲过 `get_int()` 和 `get_bool()` 的实现，这里把它们完整写出来：

```cpp
std::string IniParser::get_string(const std::string& section,
                                  const std::string& key,
                                  const std::string& default_val) const {
    auto section_it = data_.find(section);
    if (section_it == data_.end()) {
        return default_val;
    }

    auto key_it = section_it->second.find(key);
    if (key_it == section_it->second.end()) {
        return default_val;
    }

    return key_it->second;
}

int IniParser::get_int(const std::string& section,
                       const std::string& key,
                       int default_val) const {
    auto section_it = data_.find(section);
    if (section_it == data_.end()) {
        return default_val;
    }

    auto key_it = section_it->second.find(key);
    if (key_it == section_it->second.end()) {
        return default_val;
    }

    try {
        return std::stoi(key_it->second);
    } catch (const std::exception&) {
        warnings_.push_back("Invalid integer value for " + section + "." + key);
        return default_val;
    }
}

bool IniParser::get_bool(const std::string& section,
                         const std::string& key,
                         bool default_val) const {
    auto section_it = data_.find(section);
    if (section_it == data_.end()) {
        return default_val;
    }

    auto key_it = section_it->second.find(key);
    if (key_it == section_it->second.end()) {
        return default_val;
    }

    const std::string& val = key_it->second;
    std::string lower_val;
    lower_val.reserve(val.length());
    for (char c : val) {
        lower_val += static_cast<char>(std::tolower(c));
    }

    if (lower_val == "true" || lower_val == "1" ||
        lower_val == "yes" || lower_val == "on") {
        return true;
    }
    if (lower_val == "false" || lower_val == "0" ||
        lower_val == "no" || lower_val == "off") {
        return false;
    }

    warnings_.push_back("Invalid boolean value for " + section + "." + key);
    return default_val;
}
```

注意我们在类型转换失败时添加了警告信息。这样用户可以知道配置文件有问题，而不是静默地使用默认值。

### 设置值和查询接口

```cpp
void IniParser::set(const std::string& section,
                    const std::string& key,
                    const std::string& value) {
    data_[section][key] = value;
}

bool IniParser::has(const std::string& section, const std::string& key) const {
    auto section_it = data_.find(section);
    if (section_it == data_.end()) {
        return false;
    }
    return section_it->second.find(key) != section_it->second.end();
}

std::vector<std::string> IniParser::sections() const {
    std::vector<std::string> result;
    result.reserve(data_.size());
    for (const auto& [section, _] : data_) {
        result.push_back(section);
    }
    return result;
}

std::vector<std::string> IniParser::keys(const std::string& section) const {
    std::vector<std::string> result;
    auto section_it = data_.find(section);
    if (section_it == data_.end()) {
        return result;
    }
    result.reserve(section_it->second.size());
    for (const auto& [key, _] : section_it->second) {
        result.push_back(key);
    }
    return result;
}
```

这些接口很简单，主要就是哈希表的查找操作。

### 完整的头文件

让我们把完整的头文件写出来：

```cpp
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class IniParser {
public:
    IniParser() = default;

    // 文件操作
    bool load(const std::string& filename);
    bool save(const std::string& filename) const;
    bool save() const { return save(filename_); }

    // 获取值（带默认值）
    std::string get_string(const std::string& section,
                          const std::string& key,
                          const std::string& default_val = "") const;

    int get_int(const std::string& section,
                const std::string& key,
                int default_val = 0) const;

    bool get_bool(const std::string& section,
                  const std::string& key,
                  bool default_val = false) const;

    // 设置值
    void set(const std::string& section,
             const std::string& key,
             const std::string& value);

    // 查询接口
    bool has(const std::string& section, const std::string& key) const;

    std::vector<std::string> sections() const;
    std::vector<std::string> keys(const std::string& section) const;

    // 警告信息
    const std::vector<std::string>& warnings() const { return warnings_; }
    void clear_warnings() { warnings_.clear(); }

    // 获取最后加载的文件名
    const std::string& filename() const { return filename_; }

private:
    using KeyValueMap = std::unordered_map<std::string, std::string>;
    using SectionMap = std::unordered_map<std::string, KeyValueMap>;

    // 辅助函数
    std::string remove_comment(std::string line) const;
    std::string trim(std::string s) const;
    bool parse_section(const std::string& line, std::string& current_section, int line_num);
    bool parse_key_value(const std::string& line, const std::string& current_section, int line_num);

private:
    SectionMap data_;
    std::string filename_;
    mutable std::vector<std::string> warnings_;
};
```

### CMakeLists.txt

为了让项目更容易构建，我们写一个简单的 CMakeLists.txt：

```cmake
cmake_minimum_required(VERSION 3.10)
project(IniParser CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源文件
set(SOURCES
    src/IniParser.cpp
)

# 头文件
set(HEADERS
    include/IniParser.h
)

# 库
add_library(iniparser ${SOURCES} ${HEADERS})
target_include_directories(iniparser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 示例程序
add_executable(iniparser_demo examples/demo.cpp)
target_link_libraries(iniparser_demo iniparser)
```

### 测试用例

写一些测试用例来验证我们的实现：

```cpp
#include "IniParser.h"
#include <iostream>
#include <cassert>

void test_basic() {
    IniParser parser;

    // 创建一个测试配置
    parser.set("server", "host", "127.0.0.1");
    parser.set("server", "port", "8080");
    parser.set("server", "debug", "true");

    parser.set("database", "type", "sqlite");
    parser.set("database", "path", "/var/data/app.db");

    // 验证读取
    assert(parser.get_string("server", "host") == "127.0.0.1");
    assert(parser.get_int("server", "port") == 8080);
    assert(parser.get_bool("server", "debug") == true);
    assert(parser.has("server", "host") == true);
    assert(parser.has("server", "nonexistent") == false);

    // 默认值测试
    assert(parser.get_string("nonexistent", "key", "default") == "default");
    assert(parser.get_int("nonexistent", "key", 42) == 42);
    assert(parser.get_bool("nonexistent", "key", true) == true);

    std::cout << "test_basic: PASSED\n";
}

void test_type_conversion() {
    IniParser parser;

    // 各种布尔值表示
    parser.set("test", "bool1", "true");
    parser.set("test", "bool2", "false");
    parser.set("test", "bool3", "1");
    parser.set("test", "bool4", "0");
    parser.set("test", "bool5", "yes");
    parser.set("test", "bool6", "no");
    parser.set("test", "bool7", "invalid");

    assert(parser.get_bool("test", "bool1") == true);
    assert(parser.get_bool("test", "bool2") == false);
    assert(parser.get_bool("test", "bool3") == true);
    assert(parser.get_bool("test", "bool4") == false);
    assert(parser.get_bool("test", "bool5") == true);
    assert(parser.get_bool("test", "bool6") == false);
    assert(parser.get_bool("test", "bool7", true) == true);  // 默认值

    std::cout << "test_type_conversion: PASSED\n";
}

void test_sections_and_keys() {
    IniParser parser;

    parser.set("section1", "key1", "value1");
    parser.set("section1", "key2", "value2");
    parser.set("section2", "key1", "value1");

    auto sections = parser.sections();
    assert(sections.size() == 2);

    auto keys1 = parser.keys("section1");
    assert(keys1.size() == 2);

    auto keys2 = parser.keys("section2");
    assert(keys2.size() == 1);

    std::cout << "test_sections_and_keys: PASSED\n";
}

int main() {
    test_basic();
    test_type_conversion();
    test_sections_and_keys();

    std::cout << "\nAll tests passed!\n";
    return 0;
}
```

### 可能的改进

我们的 IniParser 现在已经能用了，但如果你想继续改进，这里有一些方向：

1. **保持键的顺序**：用 `std::vector<std::pair<std::string, std::string>>` 代替 `unordered_map`，保存时保持原始顺序
2. **支持转义字符**：比如 `\n`、`\t` 等
3. **支持多行值**：用反斜杠续行
4. **支持数组**：比如 `items = [a, b, c]`
5. **更宽松的语法**：支持 `key : value`、`key value` 等变体
6. **性能优化**：对于超大配置文件，可以考虑延迟加载

但这些改进会增加复杂度，对于大多数使用场景，当前的实现已经足够了。

### 小结

经过三章的努力，我们有了一个功能完整、代码清晰的 INI 解析器。在这个过程中，我们学到了：

1. 文本解析的基本思路
2. 状态机在解析中的应用
3. 类型转换和错误处理
4. CMake 工程化实践

更重要的是，我们不只是在写一个能用的工具，更理解了每一行代码背后的原理。这种深入的理解，才是学习编程最有价值的部分。

INI 文件虽然简单，但它代表了一类文本解析问题。掌握了这些技巧，以后遇到类似的任务——无论是 JSON、XML 还是自定义格式——你都能快速上手。

---

## 完结撒花

现在你可以用这个 IniParser 来管理你的应用程序配置了。它简单、可靠、完全可控，而且你知道它是怎么工作的。如果你有更复杂的需求，也可以在这个基础上扩展。

去写点需要配置文件的应用吧，你会发现有了这个 IniParser，一切都变得顺手多了。

[← 返回教程首页](README.md)
