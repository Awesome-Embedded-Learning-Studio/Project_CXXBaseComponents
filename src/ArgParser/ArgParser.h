#pragma once
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cc {

// ===================================================================
// ArgParser - 一个轻量级的 C++ 命令行参数解析器
// ===================================================================
// 支持的特性：
//   - 短参数: -o, -v
//   - 长参数: --output, --verbose
//   - 值参数: --count=10 或 --count 10
//   - 布尔开关: -v, --verbose (出现即为 true)
//   - 位置参数: 按顺序匹配的裸值
//   - 必填检查: required 参数未传入时报错
//   - 帮助信息: --help 自动生成格式化帮助
// ===================================================================

struct Arg {
  std::string short_name; // "-o"
  std::string long_name;  // "--output"
  std::string value;      // 存储的值（字符串形式）
  std::string help;       // 帮助文字
  bool required{false};   // 是否必填
  bool is_flag{false};    // 是否是布尔开关
  bool is_set{false};     // 是否已被设置
};

struct PosArg {
  std::string name;
  std::string help;
  bool required;
};

class ArgParser {
public:
  ArgParser() = default;

  // =================================================================
  // 核心接口
  // =================================================================

  /// 解析命令行参数
  void parse(int argc, char *argv[]);

  /// 注册命名参数（带值，如 -n 10 或 --count=10）
  void add_argument(const std::string &short_name, const std::string &long_name,
                    const std::string &help, bool required = false);

  /// 注册布尔开关（出现即为 true，不接值）
  void add_flag(const std::string &short_name, const std::string &long_name,
                const std::string &help);

  /// 注册位置参数（按注册顺序分配裸值）
  void add_positional(const std::string &name, const std::string &help,
                      bool required = false);

  // =================================================================
  // 读取接口
  // =================================================================

  /// 按类型获取参数值（支持 int, float, double 等支持 >> 的类型）
  template <typename T> [[nodiscard]] T get(const std::string &key) const {
    auto it = args_.find(key);
    if (it != args_.end() && !it->second.value.empty()) {
      T result;
      std::istringstream(it->second.value) >> result;
      return result;
    }

    auto pit = pos_values.find(key);
    if (pit != pos_values.end()) {
      T result;
      std::istringstream(pit->second) >> result;
      return result;
    }

    throw std::runtime_error("Argument not found: " + key);
  }

  /// 获取布尔开关的值
  [[nodiscard]] bool get_flag(const std::string &key) const;

  // 打印帮助信息到 stdout
  void print_help(std::string_view program_name) const;

private:
  Arg *findLongByShort(const std::string &name) {
    for (auto &[k, v] : args_) {
      if (v.short_name == name) {
        return &v;
      }
    }
    return nullptr;
  }

private:
  std::unordered_map<std::string, Arg> args_;      // 命名参数存储，key 为长名去掉 --
  std::vector<PosArg> pos_args;                    // 位置参数定义（按注册顺序）
  std::unordered_map<std::string, std::string> pos_values; // 位置参数值，key 为参数名
};

// =================================================================
// std::string 模板特化
// =================================================================
// 原因：默认的 istringstream 实现无法正确处理带空格的字符串
// 例如：istringstream("hello world") >> str 只会读到 "hello"
// =================================================================
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

} // namespace cc