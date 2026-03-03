// ===================================================================
// ArgParser 演示程序
// ===================================================================
// 编译: g++ demo.cpp ArgParser.cpp -o demo -std=c++23
// 用法示例:
//   ./demo input.txt -o output.txt -n 10 -v
//   ./demo input.txt --output=output.txt --count=5 --verbose
//   ./demo --help
// ===================================================================

#include "ArgParser.h"
#include <cstdlib>
#include <exception>
#include <print>

int main(int argc, char *argv[]) {
  // 创建解析器并注册参数
  cc::ArgParser parser;

  // 位置参数（必填）
  parser.add_positional("input", "输入文件路径", true);

  // 命名参数
  parser.add_argument("-o", "--output", "输出文件路径", true);
  parser.add_argument("-n", "--count", "处理数量（默认 1）", false);

  // 布尔开关
  parser.add_flag("-v", "--verbose", "启用详细输出模式");
  parser.add_flag("-f", "--force", "强制覆盖已存在文件");

  // 解析命令行参数
  try {
    parser.parse(argc, argv);
  } catch (const std::exception &e) {
    std::println(stderr, "[错误] {}", e.what());
    std::println(stderr, "使用 --help 查看帮助信息");
    return 1;
  }

  // 读取并打印参数值
  std::println("=== ArgParser 演示 ===");
  std::println("");

  // 位置参数
  try {
    const std::string input = parser.get<std::string>("input");
    std::println("输入文件:   {}", input);
  } catch (const std::exception &e) {
    std::println(stderr, "读取 input 参数失败: {}", e.what());
  }

  // 命名参数
  try {
    const std::string output = parser.get<std::string>("output");
    std::println("输出文件:   {}", output);
  } catch (const std::exception &e) {
    std::println("输出文件:   (未指定)");
  }

  // 可选的数值参数
  try {
    const int count = parser.get<int>("count");
    std::println("处理数量:   {}", count);
  } catch (const std::exception &e) {
    std::println("处理数量:   (使用默认值 1)");
  }

  // 布尔开关
  std::println("详细模式:   {}", parser.get_flag("verbose") ? "开启" : "关闭");
  std::println("强制覆盖:   {}", parser.get_flag("force") ? "开启" : "关闭");

  std::println("");
  std::println("=== 解析完成 ===");

  return 0;
}
