#include "ArgParser.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
// 将长名参数转换为 key（去掉 -- 前缀）
// 例如: "--output" -> "output"
std::string longNameToKey(const std::string &key) {
  return key.substr(2);
}

} // namespace

namespace cc {
void ArgParser::add_argument(const std::string &short_name,
                             const std::string &long_name, // --output
                             const std::string &help, bool required) {
  const std::string key = longNameToKey(long_name);
  args_[key] = {
      .short_name = short_name,
      .long_name = long_name,
      .value = "",
      .help = help,
      .required = required,
      .is_flag = false,
  };
}

void ArgParser::add_flag(const std::string &short_name,
                         const std::string &long_name,
                         const std::string &help) {
  const std::string key = longNameToKey(long_name);
  args_[key] = {
      .short_name = short_name,
      .long_name = long_name,
      .value = "",
      .help = help,
      .required = false,
      .is_flag = true,
  };
}

void ArgParser::add_positional(const std::string &name, const std::string &help,
                               bool required) {
  pos_args.emplace_back(
      PosArg{.name = name, .help = help, .required = required});
}

void ArgParser::parse(int argc, char *argv[]) {
  // 存储所有裸值（位置参数候选）
  std::vector<std::string_view> pos_arg_view;

  for (int i = 1; i < argc; i++) {
    std::string_view token{argv[i]};

    // 处理 --help 或 -h
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
        throw std::invalid_argument("Unknown argument: --" + short_key);
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

  for (size_t i = 0; i < pos_args.size(); ++i) {
    if (i < pos_arg_view.size()) {
      pos_values[pos_args[i].name] = pos_arg_view[i];
    } else if (pos_args[i].required) {
      throw std::invalid_argument("Required position args: <" +
                                  pos_args[i].name + ">");
    }
  }

  for (auto &[key, arg] : args_) {
    if (arg.required && !arg.is_set) {
      throw std::invalid_argument("Required args: " + key);
    }
  }
}

// ====================================================================
// get_flag() - 获取布尔开关的值
// ====================================================================
// 返回 true 表示开关被设置（命令行中出现了该 flag）
// 返回 false 表示开关未设置或参数不存在
bool ArgParser::get_flag(const std::string &key) const {
  auto it = args_.find(key);
  if (it != args_.end()) {
    return it->second.is_set;
  }
  return false;
}

// ====================================================================
// print_help() - 打印格式化的帮助信息
// ====================================================================
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

} // namespace cc