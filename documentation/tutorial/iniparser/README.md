# IniParser 实战教程

> **现代C++工程实践：手写一个 INI 解析器**

INI 文件是最简单直观的配置文件格式之一，虽然现在有了 JSON、YAML、TOML 等更现代的替代品，但 INI 依然在很多项目中占有一席之地。本教程将从零开始，用现代 C++ 实现一个功能完整的 INI 解析器。

---

## 教程章节

| 章节 | 标题 | 内容概要 |
|------|------|----------|
| [00](00-prerequisite.md) | 前置知识 | string_view、optional、split/trim 实现 |
| [01](01-design.md) | 需求分析与设计 | 解析目标、接口设计、数据结构 |
| [02](02-parse.md) | 解析实现 | 核心解析逻辑与状态机设计 |
| [03](03-error.md) | 错误处理 | 异常设计、边界情况处理 |

---

## 配套资源

- **视频教程**: [B站 - 现代C++工程实践](https://space.bilibili.com/294645890/lists/7045956)（12集）
- **源码位置**: [project/IniParser/](../../project/IniParser/)
- **相关教程**: [Tutorial_cpp_SimpleIniParser](https://github.com/Awesome-Embedded-Learning-Studio/Tutorial_cpp_SimpleIniParser)
- **作者**: [是的一个城管](https://space.bilibili.com/294645890)

---

## 你将学到什么

完成本教程后，你将：

- 理解配置文件解析的完整流程
- 掌握 `std::string_view` 的高效使用技巧
- 学会用 `std::optional` 处理可能不存在的值
- 理解状态机在文本解析中的应用
- 掌握 CMake 工程化实践
- 能够编写健壮的错误处理代码
