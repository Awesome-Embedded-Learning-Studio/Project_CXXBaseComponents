# 01 - 数据结构设计

## 我们先想清楚要存什么

在写第一行解析代码之前，我们需要先搞清楚一件事：命令行参数到底应该怎么存储？这个问题看似简单，但设计上的每个决策都会影响后续的代码复杂度。

### 一个命令行参数的画像

让我们先思考一下，一个命令行参数都有哪些属性：

- 它有个短名，比如 `-o`
- 它可能还有个长名，比如 `--output`
- 它可能需要带个值，比如 `--output main.cpp`，这个 `main.cpp` 就是值
- 它也可能是布尔开关，比如 `--verbose`，出现就是 true，不需要值
- 它可能是必填的，如果不传就报错
- 它还有帮助文字，用来给用户看

这么一想，每个参数实际上是一个复杂的数据结构。我们需要用一个结构体来描述它：

```cpp
struct Arg {
  std::string short_name; // "-o"
  std::string long_name;  // "--output"
  std::string value;      // 存储的值（字符串形式）
  std::string help;       // 帮助文字
  bool required{false};   // 是否必填
  bool is_flag{false};    // 是否是布尔开关
  bool is_set{false};     // 是否已被设置
};
```

这里有个细节值得注意：`is_flag` 和 `is_set` 是两个不同的布尔值。`is_flag` 表示这个参数的"类型"是开关式的，而 `is_set` 表示在解析过程中用户"有没有传"这个参数。比如 `--verbose` 是一个 flag，用户可以在命令行里传它也可以不传；如果传了，`is_set` 就变成 true。

### 参数怎么存？这是个问题

有了 `Arg` 结构体，下一步就是怎么存储一堆这样的参数。最直观的想法是用一个 `std::vector<Arg>`，但这有个问题：查找的时候得遍历整个数组，时间复杂度是 O(n)。虽然参数数量通常不多，但作为基础库，我们应该追求更好的性能。

更好的选择是用哈希表：`std::unordered_map<std::string, Arg>`。用长名作为 key（去掉 `--` 前缀），查找就是 O(1)。比如 `--output` 对应的 key 就是 `"output"`，这样用户调用 `get<std::string>("output")` 时就能快速定位。

```cpp
std::unordered_map<std::string, Arg> args_;
```

但你可能会问：短参数怎么办？比如 `-o` 和 `--output` 是同一个参数，用户可能用任意一种形式传进来。这个问题的解决方法是：存储时只用长名作为 key，短名只是 `Arg` 结构体的一个属性。当解析到短参数时，我们通过查找找到对应的 `Arg`，然后更新它。

这就需要一个辅助函数：

```cpp
Arg *findLongByShort(const std::string &name) {
  for (auto &[k, v] : args_) {
    if (v.short_name == name) {
      return &v;
    }
  }
  return nullptr;
}
```

虽然这里还是用了遍历，但只在解析短参数时调用，而且参数数量通常很少，性能影响可以忽略。解析完成后，用户通过长名获取值时就是 O(1) 的哈希查找。

### 位置参数的特殊性

命令行参数还有一类特殊情况：位置参数（positional arguments）。它们不带 `-` 前缀，按照出现顺序来匹配。比如：

```bash
./mytool input.txt output.txt
```

这里 `input.txt` 和 `output.txt` 就是位置参数。

位置参数的处理方式和命名参数不同。我们需要：

1. 预先声明位置参数的"名字"和顺序
2. 解析时把所有裸值收集起来
3. 按顺序分配给预先声明的位置参数

所以我们需要另一个结构体来描述位置参数的声明：

```cpp
struct PosArg {
  std::string name;
  std::string help;
  bool required;
};
```

然后用一个 vector 来存储声明顺序：

```cpp
std::vector<PosArg> pos_args;  // 位置参数定义（按注册顺序）
```

解析时收集到的裸值，我们用一个 unordered_map 来存储，方便后续按名字获取：

```cpp
std::unordered_map<std::string, std::string> pos_values;  // 位置参数值
```

### 完整的数据结构

现在让我们看看完整的类定义：

```cpp
class ArgParser {
public:
  ArgParser() = default;

  // 注册命名参数（带值）
  void add_argument(const std::string &short_name,
                    const std::string &long_name,
                    const std::string &help,
                    bool required = false);

  // 注册布尔开关
  void add_flag(const std::string &short_name,
                const std::string &long_name,
                const std::string &help);

  // 注册位置参数
  void add_positional(const std::string &name,
                      const std::string &help,
                      bool required = false);

  // 解析命令行参数
  void parse(int argc, char *argv[]);

  // 获取参数值
  template <typename T>
  [[nodiscard]] T get(const std::string &key) const;

  // 获取布尔开关的值
  [[nodiscard]] bool get_flag(const std::string &key) const;

  // 打印帮助信息
  void print_help(std::string_view program_name) const;

private:
  Arg *findLongByShort(const std::string &name);

private:
  std::unordered_map<std::string, Arg> args_;
  std::vector<PosArg> pos_args;
  std::unordered_map<std::string, std::string> pos_values;
};
```

注意这里用了 `[[nodiscard]]` 属性，这是 C++17 引入的，用来提醒用户不要忽略返回值。对于 `get()` 这样的函数，如果你调用了它却不使用返回值，编译器会给出警告。

### 一个设计细节：key 的处理

我们约定，存储时 key 总是长名去掉 `--` 前缀。这意味着：

- 注册参数 `--output` 时，内部存储的 key 是 `"output"`
- 解析 `--output=file` 时，提取出 `"output"` 作为 key
- 用户调用 `get<std::string>("output")` 时，直接用这个 key 查找

这个约定贯穿整个设计，保持了一致性。为了方便转换，我们有个小工具函数：

```cpp
namespace {
std::string longNameToKey(const std::string &key) {
  return key.substr(2);  // "--output" -> "output"
}
}
```

把它放在匿名命名空间里，这样它只在当前翻译单元可见，不会污染全局命名空间。

### 小结

到这里，我们的数据结构设计就完成了。这个设计有几个亮点：

1. **高效查找**：用哈希表存储命名参数，O(1) 查找
2. **统一接口**：无论短参数还是长参数，内部都用长名作为 key
3. **分离关注点**：命名参数和位置参数分开处理，逻辑清晰
4. **类型安全**：用模板提供类型安全的 `get()` 接口

但纸面设计再好，也得实际跑起来才行。下一章我们来实现最核心的 `parse()` 函数——这才是真正见功夫的地方。各种边界情况、错误处理、参数格式，都要在这里妥善处理。

[下一章：解析逻辑实现 →](02-parse.md)
