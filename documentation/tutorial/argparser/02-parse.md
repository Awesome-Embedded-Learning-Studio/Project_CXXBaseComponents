# 02 - 解析逻辑实现

## 上号！parse 函数的实现

数据结构设计好了，现在我们来实现最核心的 `parse()` 函数。这个函数的输入是标准的 `main(int argc, char* argv[])` 参数，输出是把所有参数解析好填进我们的数据结构里。

### 解析的总体思路

在写代码之前，我们先想清楚解析的流程。命令行参数是一个字符串数组，我们需要遍历这个数组，对每个 token 做分类处理：

1. 如果是 `--help` 或 `-h`，打印帮助信息并退出
2. 如果以 `--` 开头，说明是长参数
   - 如果包含 `=`，比如 `--output=file`，拆分 key 和 value
   - 如果不包含 `=`，比如 `--output file`，下一个 token 就是 value
3. 如果以 `-` 开头（但不是 `--`），说明是短参数，类似处理
4. 否则，就是位置参数，收集起来后面处理

这个流程看起来 straightforward，但里面有不少坑。我们一步步来。

### 处理 --help

先来个简单的热身：

```cpp
void ArgParser::parse(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    std::string_view token{argv[i]};

    // 处理 --help 或 -h
    if (token == "--help" || token == "-h") {
      print_help(argv[0]);
      std::exit(0);
    }

    // ... 其他处理
  }
}
```

这里用 `std::string_view` 而不是 `std::string` 是为了避免不必要的拷贝。`argv[i]` 本身就是 C 风格字符串，`string_view` 可以直接包装它而不拷贝内容。这是一个小优化，但在参数解析这种频繁调用字符串操作的场景下，积少成多。

注意我们用 `std::exit(0)` 而不是 `return`，这是因为打印帮助后我们确实想退出整个程序，而不是继续执行后续逻辑。

### 处理长参数 --key

长参数的处理逻辑稍微复杂一点，因为它有两种形式：`--key=value` 和 `--key value`。我们需要先判断有没有等号：

```cpp
// --key = value
if (token.size() > 2 && token[0] == '-' && token[1] == '-') {
  auto equal_pos = token.find('=');
  if (equal_pos != std::string::npos) {
    // --key=value 形式
    std::string_view key_view = token.substr(2, equal_pos - 2);
    std::string key{key_view};
    auto it = args_.find(std::string{key});
    if (it == args_.end()) {
      throw std::invalid_argument("Unknown argument: --" + key);
    }

    it->second.value = token.substr(equal_pos + 1);
    it->second.is_set = true;
  } else {
    // --key value 形式
    std::string_view key_view = token.substr(2);
    std::string key{key_view};
    auto it = args_.find(std::string{key});
    if (it == args_.end()) {
      throw std::invalid_argument("Unknown argument: --" + key);
    }

    if (it->second.is_flag) {
      // 如果是 flag，出现就设置为 true
      it->second.is_set = true;
    } else if (i + 1 < argc) {
      // 否则，下一个 token 是值
      it->second.value = argv[++i];  // 注意这里 i++，跳过下一个 token
      it->second.is_set = true;
    } else {
      throw std::invalid_argument("Missing value: " + std::string{token});
    }
  }
  continue;
}
```

这里有几个关键点需要注意：

首先，我们用 `token.size() > 2 && token[0] == '-' && token[1] == '-'` 来判断是否是长参数。`size() > 2` 是为了排除单独的 `--`（这通常表示"后续参数都是位置参数"的特殊标记，虽然我们这里没实现）。

其次，`--key=value` 形式下，我们用 `token.substr(2, equal_pos - 2)` 来提取 key。注意 `substr` 的第二个参数是长度，不是结束位置，所以是 `equal_pos - 2` 而不是 `equal_pos - 1`。

再次，对于 `--key value` 形式，我们用 `argv[++i]` 来获取下一个 token。这里 `++i` 是前置递增，意味着我们先让 i 加 1，然后再取 `argv[i]`。这样做的好处是循环的下一次迭代会自动跳过这个 value token，因为它已经被消费了。

最后，如果参数既不是 flag 又没有值，我们就抛出异常。这个错误处理很重要，否则用户输错了参数程序可能悄悄地继续运行，产生难以排查的 bug。

### 处理短参数 -k

短参数的处理和长参数类似，只是少了等号形式（虽然 ` -k=value` 也可以支持，但我们这里只实现 `-k value` 形式）：

```cpp
// -o
if (token.size() >= 2 && token[0] == '-') {
  std::string short_key = std::string{token};
  Arg *arg = findLongByShort(short_key);
  if (!arg) {
    throw std::invalid_argument("Unknown argument: -" + short_key);
  }

  if (arg->is_flag) {
    arg->is_set = true;
  } else if (i + 1 < argc) {
    arg->value = argv[++i];
    arg->is_set = true;
  } else {
    throw std::invalid_argument("Missing value: " + std::string{token});
  }

  continue;
}
```

这里我们用 `findLongByShort()` 来找到对应的 `Arg` 指针，然后直接修改它的内容。注意这里我们返回的是指针，所以修改是直接作用在存储的 `Arg` 对象上的。

### 收集位置参数

位置参数的处理相对简单，我们先收集所有的裸值，然后在后面按顺序分配：

```cpp
// 收集所有裸值（位置参数候选）
std::vector<std::string_view> pos_arg_view;

// ... 在解析循环中
else {
  // 既不是长参数也不是短参数，就是位置参数
  pos_arg_view.emplace_back(token);
}
```

### 分配位置参数

位置参数的分配在所有命名参数解析完成后进行：

```cpp
for (size_t i = 0; i < pos_args.size(); ++i) {
  if (i < pos_arg_view.size()) {
    // 有足够的裸值，直接分配
    pos_values[pos_args[i].name] = pos_arg_view[i];
  } else if (pos_args[i].required) {
    // 裸值不够，但这个位置参数是必填的
    throw std::invalid_argument("Required position args: <" +
                                pos_args[i].name + ">");
  }
  // 如果裸值不够且不是必填的，就留空（后续 get 时会抛异常）
}
```

这里我们用一个 for 循环遍历所有声明的位置参数。如果有足够多的裸值，就按顺序分配；如果裸值不够但这个参数是必填的，就抛异常。

### 检查必填参数

最后，我们还需要检查所有必填的命名参数是否都被设置了：

```cpp
for (auto &[key, arg] : args_) {
  if (arg.required && !arg.is_set) {
    throw std::invalid_argument("Required args: " + key);
  }
}
```

这个循环放在 parse 函数的最后，确保所有参数都解析完毕后做最终检查。

### 完整的 parse 函数

现在让我们把所有代码拼起来，看看完整的 `parse()` 函数：

```cpp
void ArgParser::parse(int argc, char *argv[]) {
  std::vector<std::string_view> pos_arg_view;

  for (int i = 1; i < argc; i++) {
    std::string_view token{argv[i]};

    if (token == "--help" || token == "-h") {
      print_help(argv[0]);
      std::exit(0);
    }

    // --key = value
    if (token.size() > 2 && token[0] == '-' && token[1] == '-') {
      auto equal_pos = token.find('=');
      if (equal_pos != std::string::npos) {
        std::string_view key_view = token.substr(2, equal_pos - 2);
        std::string key{key_view};
        auto it = args_.find(std::string{key});
        if (it == args_.end()) {
          throw std::invalid_argument("Unknown argument: --" + key);
        }
        it->second.value = token.substr(equal_pos + 1);
        it->second.is_set = true;
      } else {
        std::string_view key_view = token.substr(2);
        std::string key{key_view};
        auto it = args_.find(std::string{key});
        if (it == args_.end()) {
          throw std::invalid_argument("Unknown argument: --" + key);
        }
        if (it->second.is_flag) {
          it->second.is_set = true;
        } else if (i + 1 < argc) {
          it->second.value = argv[++i];
          it->second.is_set = true;
        } else {
          throw std::invalid_argument("Missing value: " + std::string{token});
        }
      }
      continue;
    }

    // -o
    if (token.size() >= 2 && token[0] == '-') {
      std::string short_key = std::string{token};
      Arg *arg = findLongByShort(short_key);
      if (!arg) {
        throw std::invalid_argument("Unknown argument: -" + short_key);
      }
      if (arg->is_flag) {
        arg->is_set = true;
      } else if (i + 1 < argc) {
        arg->value = argv[++i];
        arg->is_set = true;
      } else {
        throw std::invalid_argument("Missing value: " + std::string{token});
      }
      continue;
    }

    pos_arg_view.emplace_back(token);
  }

  // 分配位置参数
  for (size_t i = 0; i < pos_args.size(); ++i) {
    if (i < pos_arg_view.size()) {
      pos_values[pos_args[i].name] = pos_arg_view[i];
    } else if (pos_args[i].required) {
      throw std::invalid_argument("Required position args: <" +
                                  pos_args[i].name + ">");
    }
  }

  // 检查必填参数
  for (auto &[key, arg] : args_) {
    if (arg.required && !arg.is_set) {
      throw std::invalid_argument("Required args: " + key);
    }
  }
}
```

### 一点踩坑预警

这里有个容易踩的坑：我们用 `std::string_view` 来避免拷贝，但 `string_view` 只是个视图，它不拥有字符串的内容。如果原始字符串被销毁，`string_view` 就会变成悬垂指针。

在我们的代码里这是安全的，因为 `argv[]` 的生命周期贯穿整个 `main` 函数，`pos_arg_view` 也在 `parse()` 函数内部使用，没有逃逸到外部。但如果你把 `string_view` 存储到类成员变量里，然后原始字符串被销毁了，那就会出问题。

### 小结

到这里，ArgParser 的核心解析逻辑就完成了。这个函数虽然只有几十行，但处理了命令行参数的各种情况。从 `--help` 的特殊处理，到长参数的两种形式，再到短参数和位置参数，每一步都有其考虑。

下一章我们来实现 `get()` 模板函数和帮助信息，这两个功能会让我们的 ArgParser 真正变得实用。

[下一章：模板特化与帮助信息 →](03-advanced.md)
