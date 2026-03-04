# 01 - 需求分析与设计

## 我们要构建什么

在写第一行代码之前，我们需要先搞清楚：INI 解析器到底要做什么？要做到什么程度？这些问题看似简单，但不同的回答会导致完全不同的设计。

### INI 文件格式回顾

让我们先看一个典型的 INI 文件：

```ini
; 这是注释
[server]
host = 127.0.0.1
port = 8080
debug = true

[database]
type = sqlite
path = /var/data/app.db
max_connections = 100
```

INI 文件的格式规则：

1. **注释**：以 `;` 或 `#` 开头
2. **节（Section）**：用 `[section_name]` 表示
3. **键值对**：`key = value` 格式，等号两边可有可无空格
4. **全局键值对**：任何节之前的键值对属于"全局节"
5. **大小写敏感**：不同实现有不同约定，我们约定节名和键名都保留大小写
6. **值的类型**：都是字符串，使用时按需转换

### 我们的支持范围

实现一个完全符合所有 INI 变体的解析器是个无底洞，我们明确一下边界：

**支持的功能**：
- 注释（`;` 和 `#`）
- 节（`[section]`）
- 键值对（`key = value`）
- 键值对前后空格自动去除
- 值内可包含空格
- 空行忽略
- 多个相同的 key：后出现的覆盖先出现的

**不支持的功能**：
- 转义字符（如 `\n`）
- 多行值（用反斜杠续行）
- 内嵌变量（`${var}`）
- 数组/列表类型

这些"不支持"的功能不是不能做，而是增加了复杂度。对于大多数使用场景，我们支持的功能已经足够了。

### 接口设计

从使用者的角度，我们希望这样使用解析器：

```cpp
IniParser parser;
if (!parser.load("config.ini")) {
    std::cerr << "Failed to load config\n";
    return 1;
}

// 获取字符串值
std::string host = parser.get_string("server", "host", "localhost");
int port = parser.get_int("server", "port", 8080);

// 检查 key 是否存在
if (parser.has("server", "debug")) {
    bool debug = parser.get_bool("server", "debug", false);
}

// 设置值
parser.set("server", "port", "9090");

// 保存回文件
parser.save("config_new.ini");
```

注意我们的设计：`get_xxx()` 函数接受一个默认值。如果 key 不存在，返回默认值而不是抛异常。这种设计在配置文件场景下更实用——配置文件通常有合理的默认值，缺少某个配置不应该导致程序崩溃。

### 类结构设计

基于上面的接口，我们设计类结构：

```cpp
class IniParser {
public:
    // 加载文件
    bool load(const std::string& filename);

    // 保存到文件
    bool save(const std::string& filename) const;

    // 获取值（带默认值）
    std::string get_string(const std::string& section,
                          const std::string& key,
                          const std::string& default_val) const;

    int get_int(const std::string& section,
                const std::string& key,
                int default_val) const;

    bool get_bool(const std::string& section,
                  const std::string& key,
                  bool default_val) const;

    // 检查 key 是否存在
    bool has(const std::string& section, const std::string& key) const;

    // 设置值
    void set(const std::string& section,
             const std::string& key,
             const std::string& value);

    // 获取所有节名
    std::vector<std::string> sections() const;

    // 获取节内所有键名
    std::vector<std::string> keys(const std::string& section) const;

private:
    // 内部数据结构
    using KeyValueMap = std::unordered_map<std::string, std::string>;
    using SectionMap = std::unordered_map<std::string, KeyValueMap>;

    SectionMap data_;
    std::string filename_;  // 最后加载的文件名
};
```

### 数据结构选择

我们用嵌套的哈希表来存储数据：`SectionMap` 是节名到键值对的映射，每个 `KeyValueMap` 是键到值的映射。

```cpp
std::unordered_map<std::string,       // 节名
    std::unordered_map<std::string,   // 键名
        std::string>>                 // 值
    data_;
```

选择 `std::unordered_map` 而不是 `std::map` 是因为我们：

1. 不需要有序访问
2. O(1) 查找比 O(log n) 更快
3. 内存占用更小

但如果你需要保持 INI 文件中键的顺序（比如某些配置文件格式要求），可以用 `std::map` 或 `std::unordered_map` + `std::vector` 组合。

### 解析流程设计

解析 INI 文件是个典型的"逐行处理 + 状态机"问题：

```
开始
  ↓
读取一行
  ↓
去掉注释内容
  ↓
trim 前后空白
  ↓
是否为空？→ 是 → 跳过
  ↓ 否
是否以 [ 开头？→ 是 → 解析节名，切换当前节
  ↓ 否
是否包含 = ？→ 是 → 解析键值对，存入当前节
  ↓ 否
语法错误？
```

我们需要一个"当前节"的概念。当解析到 `[server]` 时，切换当前节为 `server`。后续的键值对都存入 `server` 节。在遇到下一个节声明之前，所有键值对都属于当前节。

对于"全局键值对"（任何节之前的），我们可以用一个特殊的节名比如 `""` 或 `"$global$"` 来存储。

### 错误处理策略

对于错误处理，我们有几个选择：

1. **宽松模式**：跳过无法解析的行，继续处理
2. **严格模式**：遇到错误立即停止
3. **警告模式**：记录错误但继续处理

我们选择"警告模式"：解析时收集警告信息，解析完成后可以通过某个接口获取。这样既不会因为一个小错误导致整个配置无法加载，又能让用户知道有什么问题。

```cpp
class IniParser {
public:
    // ...

    // 获取解析过程中的警告
    const std::vector<std::string>& warnings() const { return warnings_; }

    // 清空警告
    void clear_warnings() { warnings_.clear(); }

private:
    std::vector<std::string> warnings_;
};
```

### 类型转换的细节

在实现 `get_int()` 和 `get_bool()` 时，我们需要处理类型转换：

```cpp
int IniParser::get_int(const std::string& section,
                       const std::string& key,
                       int default_val) const {
    auto it = data_.find(section);
    if (it == data_.end()) return default_val;

    auto it2 = it->second.find(key);
    if (it2 == it->second.end()) return default_val;

    try {
        return std::stoi(it2->second);
    } catch (...) {
        return default_val;  // 转换失败，返回默认值
    }
}
```

对于 `get_bool()`，我们需要定义什么是"布尔值"：

```cpp
bool IniParser::get_bool(const std::string& section,
                         const std::string& key,
                         bool default_val) const {
    // ... 查找逻辑 ...

    const std::string& val = it2->second;
    // 转小写比较
    std::string lower_val;
    lower_val.reserve(val.length());
    for (char c : val) {
        lower_val += static_cast<char>(std::tolower(c));
    }

    if (lower_val == "true" || lower_val == "1" || lower_val == "yes" || lower_val == "on") {
        return true;
    }
    if (lower_val == "false" || lower_val == "0" || lower_val == "no" || lower_val == "off") {
        return false;
    }

    return default_val;  // 无法识别
}
```

这里我们支持常见的布尔值表示：`true/false`、`1/0`、`yes/no`、`on/off`。

### 小结

到这里，我们的设计就基本完成了。我们明确了：

1. 支持 INI 格式的哪些特性
2. 不支持哪些特性
3. 用户接口是什么样的
4. 内部数据结构如何选择
5. 解析流程的框架
6. 错误处理策略
7. 类型转换的细节

设计阶段花的时间越充分，编码时就越顺畅。磨刀不误砍柴工，说的就是这个道理。

下一章我们开始实现核心的解析逻辑，这可是最见功夫的部分。

[下一章：解析实现 →](02-parse.md)
