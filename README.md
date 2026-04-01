# 🎬 Project_CXXBaseComponents

**C++23 | CMake 3.25+ | MIT License**

> **用工程实践的方式学 C++，而不只是刷语法。**

本仓库是 B 站系列教程 **[现代C++工程实践](https://space.bilibili.com/294645890/lists/7045956)** 的配套代码仓库。

---

## ✨ 为什么这个教程

如果你：
- 会C++了，但是不知道拿他干啥
- 写了一段时间 C++，却不知道如何组织"真正的工程"
- 想学现代 C++ 特性（C++11 ~ C++23），但找不到合适的实战项目

说不定，这个教程适合你！笔者希望不只是教语法，而是手带你走过从需求分析、设计方案、到编码实现、再到工程落地的完整过程。每一步"为什么这样设计"都有交代。

---

## 📦 快速开始

### 环境要求

| 项目 | 最低版本 | 推荐版本 |
|------|----------|----------|
| C++ 标准 | C++23 | C++23 |
| GCC | 11+ | 13+ |
| Clang | 13+ | 16+ |
| MSVC | 193+ | 最新 |
| CMake | 3.25+ | 3.28+ |

### 克隆仓库

```bash
# 克隆主仓库
git clone https://github.com/Awesome-Embedded-Learning-Studio/Project_CXXBaseComponents
cd Project_CXXBaseComponents

# 初始化子模块（包含外部依赖库）
git submodule update --init --recursive
```

### 选择你的第一个项目

如果你是 C++ 初学者，建议按以下顺序学习：

| 项目 | 难度 | 耗时 | 你将学到 |
|------|:----:|------|----------|
| **[ArgParser](./src/ArgParser/)** | 🌱 | ~2h | 命令行参数解析、模板编程、STL 容器、异常处理 |
| **[FileCopier](./src/FileCopier/)** | 🌱 | ~2h | 文件操作、进度条显示、性能测量 |
| **[IniParser](./project/IniParser/)** | ⚡ | ~6h | `string_view`、`optional`、字符串处理、CMake |
| **[MemoryPool](./project/memory_pool/)** | 🔥 | ~8h | 内存管理、线程安全、性能优化、benchmark |
| **[Mimalloc](./project/external/mimalloc/)** | 💎 | ~4h | 开源项目源码阅读、高级内存分配器设计 |

### 构建与运行

每个子项目都是独立的 CMake 工程，可以在对应目录下单独构建：

```bash
# 以 ArgParser 为例
cd src/ArgParser
cmake -B build
cmake --build build

# 运行演示程序
./build/demo --help
```

> **说明**
> - `src/` 下每个目录均为独立的 CMake 工程，可单独编译运行
> - 体量较大的项目以 **Git Submodule** 形式链接外部仓库
> - `documentation/` 中存放对应项目的知识点梳理与补充文档

---

## 📚 项目清单

> **完整发展规划：** 参见 [ROADMAP.md](./ROADMAP.md) — 35 章课程大纲 + 已完成项目映射 + sysmon 贯穿项目时间线

**难度说明**：🌱 入门 | ⚡ 初级 | 🔥 中级 | 💎 进阶

| 项目名 | 一句话简介 | 路径 | 视频 | 文档 | 状态 | 难度 |
|--------|-----------|------|------|------|------|------|
| **ArgParser** | 从零实现命令行参数解析器 | `src/ArgParser/` | [📺](./video/argparser.md) | [📄](./documentation/tutorial/ArgParser/) | ✅ | 🌱 |
| **FileCopier** | 带进度条的文件拷贝工具 | `src/FileCopier/` | [📺](./video/filecopier.md) | - | ✅ | 🌱 |
| **IniParser** | INI 配置文件解析器 | `project/IniParser/` | [📺](./video/iniparser.md) | [📄](./project/IniParser/tutorial/) | ✅ | ⚡ |
| **MemoryPool** | 高性能内存池实现 | `project/memory_pool/` | [📺](./video/memory_pool.md) | - | ✅ | 🔥 |
| **Mimalloc** | 微软开源分配器源码阅读 | `project/external/mimalloc/` | [📺](./video/mimalloc.md) | - | ✅ | 💎 |

### 📦 外部子模块

| 路径 | 仓库 | 说明 |
|------|------|------|
| [project/external/mimalloc](./project/external/mimalloc) | [microsoft/mimalloc](https://github.com/microsoft/mimalloc) | 高性能内存分配器 |
| [project/memory_pool](./project/memory_pool) | [Project_MakeAMemroyPool](https://github.com/Awesome-Embedded-Learning-Studio/Project_MakeAMemroyPool) | 内存池实现教程 |
| [project/IniParser](./project/IniParser) | [Tutorial_cpp_SimpleIniParser](https://github.com/Awesome-Embedded-Learning-Studio/Tutorial_cpp_SimpleIniParser) | INI配置文件解析器 |

---

## 📺 视频系列

### 🎬 现代C++工程实践

👉 **[是的一个城管](https://space.bilibili.com/294645890)**

| 项目 | 视频数 | 专题列表 |
|------|--------|----------|
| [ArgParser](./video/argparser.md) | 3 | 命令行参数解析器 |
| [IniParser](./video/iniparser.md) | 12 | INI配置文件解析器 |
| [FileCopier](./video/filecopier.md) | 5 | 文件拷贝与进度条 |
| [MemoryPool](./video/memory_pool.md) | 9 | 高性能内存池实现 |
| [Mimalloc](./video/mimalloc.md) | 5 | 开源项目源码阅读 |

<details>
<summary><b>📝 完整播放列表</b></summary>

**[现代C++工程实践](https://space.bilibili.com/294645890/lists/7045956)** - 持续更新中

</details>

---

## 📖 文档说明

### 教程文档

`documentation/tutorial/` 目录下每个项目配套独立的 Markdown 文档，内容包括：

- **动机篇**：为什么需要这个组件？解决了什么问题？
- **设计篇**：数据结构如何设计？有哪些权衡？
- **实现篇**：核心逻辑的实现细节
- **回顾篇**：总结与扩展方向

每篇文档都对应视频的一个章节，既可以配合视频学习，也可以作为独立的技术文章阅读。

### 视频目录

`video/` 目录下存放各项目的视频列表和学习路线：

- [video/argparser.md](./video/argparser.md) - ArgParser 系列视频
- [video/filecopier.md](./video/filecopier.md) - FileCopier 系列视频
- [video/iniparser.md](./video/iniparser.md) - IniParser 系列视频
- [video/memory_pool.md](./video/memory_pool.md) - MemoryPool 系列视频
- [video/mimalloc.md](./video/mimalloc.md) - Mimalloc 源码阅读视频

---

## 🤝 参与贡献

欢迎提交 Issue 和 Pull Request！

### 反馈与建议

如果你在学习过程中遇到问题：

- 📺 **B站视频留言** - 响应最快，优先处理
- 🐛 **GitHub Issue** - 描述具体问题，附上复现代码
- 💬 **讨论区** - 交流学习心得，提出建议

非常感谢来自B站评论区的各位的建议，这里特别对各位的建议整理成一份TODO清单:

| 平台 | 用户名 | 原评论 | 对应的TODO反馈 |
|------|--------|--------|----------------|
| B站 | cache是什么 | 来自c++26有些新特性有助于写一个更好用的argparser，可以等编译器支持了再写一个，比如反射机制 | 等gcc足够新的支持静态反射，重新出一版ArgParser教程 |

### 贡献方式

- 修正文档错别字或表述不清之处
- 补充更多使用示例
- 优化代码实现或添加测试用例
- 提出新项目建议

---

## 📜 许可证

本仓库代码以 [MIT License](./LICENSE) 开源，可自由使用、修改和分发。

文档内容保留所有权利，转载请注明出处。

---

## 🌟 如果对你有帮助

如果这个项目对你有帮助，欢迎：

- 点个 ⭐ Star，这是对我最大的鼓励
- 分享给身边学习 C++ 的朋友（然后一起开喷代码写的好烂）（逃
- 在 B 站关注我，获取更新通知

## 🌟 其他

当然如果感觉自己误闯天家，没事，这里还有专门的（偏嵌入式的）现代C++教程，点击访问仓库：

👉 :link: [现代C++教程](https://github.com/Awesome-Embedded-Learning-Studio/Tutorial_AwesomeModernCPP)

---

<p align="center">
  <i>"代码不仅是写出来的，更是设计出来的。"</i><br>
  <i>"把简单的事情做好，就是不简单。"</i><br><br>
  <a href="https://space.bilibili.com/294645890">📺 B站：是的一个城管</a>
</p>
