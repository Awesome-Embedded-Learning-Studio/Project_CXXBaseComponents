# ROADMAP — 现代 C++ 工程实践 发展路线图

> **定位：** 从"能写 C++ 代码"到"能用 C++ 独立解决真实工程问题"
> **技术选型：** C++17 基础 + C++20/23 按需引入 | CMake 全套 | GCC/Clang/MSVC | GDB/Valgrind/Sanitizer

---

## 学习路径总览

```
Part 0          Part 1              Part 2              Part 3
启程            语言层              工程层              系统层
Ch00-02         Ch03-12             Ch13-25             Ch26-35
(3章)           (10章)              (13章)              (10章)
 │               │                   │                   │
 └───────────────┴───────────────────┴───────────────────┤
                                                         │
                      ┌──────────────────────────────────┤
                      │                                  │
                 Part 4: 网络          Part 5: 高级特性   │
                 Ch36-41 (6章)         Ch42-49 (8章)     │
                      │                   │              │
                      └─────────┬─────────┘              │
                                │                        │
                          Part 6: 生产工程                │
                          Ch50-57 (8章)                  │
                                │                        │
                          Part 7: 整合构建                │
                          Ch58-62 (5章) ◄────────────────┘
```

**状态说明：** `DONE` = 教程+代码+视频完成 | `PROJECT` = 有已完成项目可作素材 | `PLANNED` = 大纲已有，待落地

---

## Part 0：启程

> **你将获得什么：** 搭好开发环境，跑通第一个 CMake 工程，为后续所有章节做好准备。
> **预计总耗时：** ~5h

| 章节 | 标题 | 状态 | 关联项目 | 迷你项目 | sysmon 里程碑 | ⏱ | 核心收获 |
|------|------|:----:|----------|---------|---------------|----|----------|
| Ch00 | 为什么你的 C++ 需要重新学 | PLANNED | — | `env_check` | 介绍 sysmon 最终形态 | ~1h | 重新认识 C++ 的工程价值 |
| Ch01 | 开发环境搭建 | PLANNED | — | `hello_cmake` | 建仓库推第一个 commit | ~2h | 工欲善其事，必先利其器 |
| Ch02 | CMake 基础 | PLANNED | — | `multi_target` | sysmon CMake 骨架 | ~2h | 用 CMake 管理多目标工程 |

---

## Part 1：语言层 — Modern C++ 系统导读

> **你将获得什么：** 掌握 C++ 核心语言机制，写出安全、高效、可维护的现代 C++ 代码。
> **前置要求：** Part 0
> **预计总耗时：** ~33h

| 章节 | 标题 | 状态 | 关联项目 | 迷你项目 | sysmon 里程碑 | ⏱ | 核心收获 |
|------|------|:----:|----------|---------|---------------|----|----------|
| Ch03 | 值类型与所有权（move semantics） | PLANNED | — | `move_buffer` | 数据包 move 传递 | ~3h | 理解左值/右值，写出零拷贝代码 |
| Ch04 | RAII | PROJECT | **FileCopier** | `scoped_timer` | /proc 读取封 RAII | ~3h | 用析构函数管理一切资源 |
| Ch05 | 智能指针 | PLANNED | — | `smart_zoo` | 模块用 unique_ptr 持有 | ~3h | 彻底告别手动 new/delete |
| Ch06 | 泛型编程（template/concepts） | PROJECT | **ArgParser** | `type_printer` | 格式化模块泛型化 | ~4h | 写出类型安全且通用的代码 |
| Ch07 | 标准库精讲 | PROJECT | **FileCopier**, DirScanner | `file_stat` | 路径/时间戳标准化 | ~4h | 掌握 STL 的精髓与陷阱 |
| Ch08 | 并发基础 | PROJECT | **MemoryPool** | `parallel_wc` | 采样线程通信 | ~4h | 写出正确的多线程程序 |
| Ch09 | 语言级地雷（UB/生命周期） | PLANNED | — | `ub_hunter` | 全库 ASan 扫描 | ~2h | 识别并避开 C++ 的暗坑 |
| Ch10 | CMake 进阶（install/export） | PLANNED | — | `mylib` | core 库拆分 install | ~3h | 让你的库可以被别人用 |
| **Ch11** | **错误处理体系** | PLANNED | — | `result_type` | 采集错误分类 | ~3h | exceptions/error_code/expected 三选一 |
| **Ch12** | **类型擦除与类型安全** | PLANNED | — | `any_func` | 插件接口 type erasure | ~3h | 理解 variant/any/function 底层原理 |

> **Ch11 覆盖要点：** 异常机制原理与开销分析 · `std::error_code`/`std::error_category` · C++23 `std::expected<T,E>` · `noexcept` 与异常安全保证 · 三种错误处理策略的选择标准

> **Ch12 覆盖要点：** `std::variant` + `std::visit` · `std::any` 内部原理 · `std::function` 与 Small Buffer Optimization · 手写 type erasure · `std::invoke` 统一调用

---

## Part 2：工程层 — 构建可维护系统

> **你将获得什么：** 掌握现代 C++ 工程的全套工具链和基础设施，能独立搭建 CI/CD、包管理、日志、配置、线程池等核心模块。
> **前置要求：** Part 1
> **预计总耗时：** ~40h

| 章节 | 标题 | 状态 | 关联项目 | 迷你项目 | sysmon 里程碑 | ⏱ | 核心收获 |
|------|------|:----:|----------|---------|---------------|----|----------|
| Ch13 | CMake 全套工程（CI/CTest/CPack） | PLANNED | — | `ci_skeleton` | 接入 CI | ~3h | 一键构建、测试、打包 |
| Ch14 | Sanitizer 全家桶 | PLANNED | — | `sanitizer_lab` | 全库 Sanitizer 检查 | ~3h | 内存/线程/UB 错误自动检出 |
| Ch15 | 代码质量工具（clang-format/tidy） | PLANNED | — | `lint_clean` | sysmon 加 lint CI | ~3h | 自动化代码风格和质量检查 |
| Ch16 | C++ 包管理 — vcpkg | PLANNED | — | `vcpkg_demo` | sysmon 用 vcpkg 管依赖 | ~3h | 用 manifest 模式管理第三方依赖 |
| Ch17 | C++ 包管理 — Conan | PLANNED | — | `conan_demo` | — | ~3h | 理解 Conan 2 的 recipe 模型 |
| Ch18 | Docker 构建环境 | PLANNED | — | `docker_build` | CI 加 Docker 构建 | ~3h | 可复现的容器化构建环境 |
| Ch19 | 项目结构设计（分层架构） | PLANNED | — | `arch_demo` | 重构为四层结构 | ~3h | core/adapter/infra/app 分层实践 |
| Ch20 | 手写日志系统 | PLANNED | — | `tiny_log` | 替换所有 printf | ~3h | 格式化、分级、多线程安全的日志 |
| Ch21 | 手写配置系统 | PROJECT | **IniParser** | `config_hot` | 配置驱动采样 | ~3h | INI/YAML 解析 + 热更新 |
| Ch22 | 手写线程池 | PLANNED | — | `thread_pool` | 多路采集并行 | ~4h | 条件变量、future/promise、work-stealing |
| Ch23 | CLI 框架设计 | PROJECT | **ArgParser** | `minicli` | sysmon CLI 接口 | ~3h | 子命令、参数注册、帮助生成 |
| Ch24 | 测试体系（gtest/mock） | PLANNED | — | `test_suite` | 全模块测试套件 | ~3h | 单元测试、集成测试、mock |
| Ch25 | 模块化与 API 设计 | PLANNED | — | `pimpl_lib` | core 库 API 审查 | ~3h | Pimpl、头文件卫生、ABI 隔离 |

---

## Part 3：系统层 — 真正懂系统

> **你将获得什么：** 深入理解编译链接原理、内存模型、系统 I/O、性能优化，写出和操作系统高效协作的代码。
> **前置要求：** Part 2
> **预计总耗时：** ~36h

| 章节 | 标题 | 状态 | 关联项目 | 迷你项目 | sysmon 里程碑 | ⏱ | 核心收获 |
|------|------|:----:|----------|---------|---------------|----|----------|
| **Ch26** | **编译链接原理** | PLANNED | — | `symbol_inspect` | 理解 sysmon 符号导出 | ~4h | 理解编译器做了什么、链接器如何工作 |
| Ch27 | 动态库与插件系统 | PLANNED | — | `plugin_hello` | 改造为插件化架构 | ~3h | 用 dlopen 实现插件化架构 |
| Ch28 | 文件系统接口与跨平台 | PLANNED | DirScanner | `file_watcher` | 配置热更新完成 | ~3h | 跨平台文件操作不再踩坑 |
| Ch29 | 系统采集 — CPU/内存 | PLANNED | — | `cpu_sampler` | cpu/mem 插件 | ~3h | 读取 /proc 实现 Linux 系统监控 |
| Ch30 | 系统采集 — 网络/磁盘 | PLANNED | — | `disk_info` | net/disk 插件 | ~3h | 监控网络和磁盘 I/O |
| Ch31 | IO 模型与事件循环 | PLANNED | — | `echo_epoll` | 定时器改 timerfd | ~4h | epoll/select/poll 原理与实战 |
| Ch32 | 并发进阶（无锁/死锁分析） | PROJECT | **MemoryPool**, **Mimalloc** | `deadlock_gdb` | 全库并发审查 | ~4h | 写出高性能无锁数据结构 |
| **Ch33** | **C++ 内存模型与原子操作** | PLANNED | — | `lock_free_stack` | 采集计数器原子化 | ~4h | 理解 happens-before 和内存序 |
| **Ch34** | **性能分析入门** | PLANNED | — | `flame_graph` | sysmon 热点优化 | ~3h | 用 perf/Valgrind 找到性能瓶颈 |
| **Ch35** | **SIMD 与向量化编程** | PLANNED | — | `simd_sum` | 数据聚合向量化 | ~4h | 利用 CPU 向量指令加速 4-16 倍 |

> **Ch26 编译链接原理 覆盖要点：** 预处理→编译→汇编→链接四阶段 · 符号表与重定位 · 静态库 vs 动态库 · name mangling · 符号可见性（`__attribute__((visibility))`）· One Definition Rule

> **Ch33 C++ 内存模型 覆盖要点：** happens-before 关系 · 6 种内存序（relaxed→seq_cst）· `std::atomic<T>` · lock-free vs wait-free · C++20 `std::latch`/`barrier`/`semaphore`

> **Ch35 SIMD 覆盖要点：** SSE/AVX/NEON 指令集 · 自动向量化条件 · SoA vs AoS 数据布局 · `std::experimental::simd` · 手写 SIMD 优化实例

---

## Part 4：网络与通信

> **你将获得什么：** 从 Socket 基础到 HTTP 服务器、序列化框架、RPC 实现，掌握网络编程全链路。
> **前置要求：** Part 3（Ch31 IO 模型为前置核心）
> **预计总耗时：** ~22h

| 章节 | 标题 | 状态 | 关联项目 | 迷你项目 | sysmon 里程碑 | ⏱ | 核心收获 |
|------|------|:----:|----------|---------|---------------|----|----------|
| **Ch36** | **Socket 编程基础** | PLANNED | — | `tcp_chat` | — | ~3h | TCP 三次握手在代码层面长什么样 |
| **Ch37** | **HTTP 协议与服务器** | PLANNED | — | `mini_httpd` | sysmon HTTP 上报接口 | ~4h | 从零实现 HTTP/1.1 服务器 |
| **Ch38** | **序列化框架设计（JSON）** | PLANNED | — | `json_parser` | 监控数据 JSON 输出 | ~4h | 递归下降解析 JSON，理解编码/解码 |
| **Ch39** | **二进制序列化与跨平台** | PLANNED | — | `binary_codec` | 高效二进制采集数据格式 | ~3h | 处理字节序、对齐、版本兼容 |
| **Ch40** | **RPC 框架入门** | PLANNED | — | `tiny_rpc` | sysmon 远程查询接口 | ~4h | 实现一个最简 RPC：序列化+网络+分发 |
| **Ch41** | **异步 I/O 进阶（io_uring）** | PLANNED | — | `uring_echo` | sysmon 高性能采集后端 | ~4h | Linux 5.1+ 新一代异步 I/O |

---

## Part 5：高级语言特性

> **你将获得什么：** 掌握模板元编程、C++20 协程、设计模式现代写法，能读懂 STL 和 Boost 源码。
> **前置要求：** Part 1 + Part 3（Ch32 并发进阶为协程前置）
> **预计总耗时：** ~29h

| 章节 | 标题 | 状态 | 关联项目 | 迷你项目 | sysmon 里程碑 | ⏱ | 核心收获 |
|------|------|:----:|----------|---------|---------------|----|----------|
| **Ch42** | **模板元编程深入** | PLANNED | — | `type_traits_lab` | 类型特征工具集 | ~5h | SFINAE、concepts、编译期计算 |
| **Ch43** | **C++20 协程** | PLANNED | — | `coro_gen` | — | ~4h | 理解协程帧、co_await、promise_type |
| **Ch44** | **协程实战 — 异步任务链** | PLANNED | — | `async_http` | sysmon 异步采集引擎 | ~4h | 用协程重写网络代码，告别回调地狱 |
| **Ch45** | **设计模式与现代 C++** | PLANNED | — | `pattern_lab` | sysmon 设计模式审查 | ~4h | 23 种模式用 C++17/20 重写 |
| **Ch46** | **依赖注入与可测试架构** | PLANNED | — | `di_demo` | sysmon 可测试化重构 | ~3h | 不用框架也能写出可测试代码 |
| **Ch47** | **自定义分配器与 PMR** | PLANNED | — | `arena_alloc` | 采集缓冲区 PMR 化 | ~4h | 掌握 C++17 PMR 和内存策略 |
| **Ch48** | **字符串处理与文本解析** | PLANNED | — | `csv_parser` | 日志格式化引擎 | ~3h | string_view、正则、编译期字符串 |
| **Ch49** | **C++26 新特性展望** | PLANNED | — | `reflection_demo` | — | ~2h | 静态反射、Contracts、Senders/Receivers |

---

## Part 6：生产级工程实践

> **你将获得什么：** 掌握 ABI 管理、安全编码、Fuzzing、FFI 互操作、深度调试等生产环境必备技能。
> **前置要求：** Part 2 + Part 3
> **预计总耗时：** ~26h

| 章节 | 标题 | 状态 | 关联项目 | 迷你项目 | sysmon 里程碑 | ⏱ | 核心收获 |
|------|------|:----:|----------|---------|---------------|----|----------|
| **Ch50** | **ABI 稳定性与符号管理** | PLANNED | — | `stable_api` | sysmon 公共 API 稳定化 | ~3h | Pimpl、符号可见性、语义版本控制 |
| **Ch51** | **代码审查与自动化重构** | PLANNED | — | `clang_tidy_rule` | sysmon 代码规范落地 | ~3h | 写自定义 clang-tidy 规则 |
| **Ch52** | **安全编码实践** | PLANNED | — | `safe_handler` | 输入校验与安全审计 | ~3h | CERT C++、缓冲区溢出防护 |
| **Ch53** | **Fuzzing 测试** | PLANNED | — | `fuzz_target` | 解析器 fuzzing | ~3h | 用 libFuzzer 找到隐藏 bug |
| **Ch54** | **FFI 与语言互操作** | PLANNED | — | `pybind_hello` | sysmon Python 绑定 | ~3h | C++/Python/Rust/WASM 互操作 |
| **Ch55** | **调试深度指南** | PLANNED | — | `core_dump` | 全库调试策略 | ~4h | GDB 高级、core dump 分析、远程调试 |
| **Ch56** | **可观测性与监控** | PLANNED | — | `otel_demo` | sysmon 自监控 | ~3h | 结构化日志、OpenTelemetry 接入 |
| **Ch57** | **嵌入式 C++ 基础** | PLANNED | — | `bare_metal` | — | ~4h | 裸机 C++：无 RTTI、无异常、constexpr |

---

## Part 7：整合 — 构建完整系统

> **你将获得什么：** 将所有模块组装成可运行的完整系统，经历架构设计→引擎组装→UI 输出→打包发布→性能调优的全流程。
> **前置要求：** Part 3（核心）+ Part 4/5/6（按需选学）
> **预计总耗时：** ~16h

| 章节 | 标题 | 状态 | 关联项目 | 迷你项目 | sysmon 里程碑 | ⏱ | 核心收获 |
|------|------|:----:|----------|---------|---------------|----|----------|
| Ch58 | 大项目架构设计 | PLANNED | — | `sysmon_arch` | sysmon 架构蓝图 | ~3h | 分层架构、模块划分、接口定义 |
| Ch59 | 整合 — 核心引擎 | PLANNED | — | `engine_assemble` | sysmon 可运行 | ~4h | 将所有模块组装成可运行系统 |
| Ch60 | 整合 — 输出层与终端 UI | PLANNED | — | `tui_dashboard` | 终端实时可视化 | ~4h | ANSI escape / FTXUI 实时 Dashboard |
| Ch61 | 整合 — CI/CD 与打包发布 | PLANNED | — | `release_pkg` | 自动发布 | ~3h | 自动化构建、测试、发布流水线 |
| Ch62 | 回顾与进阶方向 | PLANNED | — | `portfolio` | 作品集就绪 | ~2h | 四条进阶路线规划 |

---

## 已完成项目清单

> **推荐学习顺序：** ArgParser → FileCopier → DirScanner → IniParser → MemoryPool → Mimalloc

### ArgParser — 命令行参数解析器
- **路径：** `src/ArgParser/`
- **难度：** 🌱 入门 · **视频：** 3 期 · **教程：** `documentation/tutorial/ArgParser/`
- **知识点：** `命令行解析` `模板编程` `STL容器` `异常处理` `子命令分发`
- **主映射：** Ch23（CLI 框架设计）· **辅映射：** Ch06（泛型编程）

### FileCopier — 带进度条的文件拷贝工具
- **路径：** `src/FileCopier/`
- **难度：** 🌱 入门 · **视频：** 5 期
- **知识点：** `文件操作` `RAII` `进度条` `std::chrono` `std::filesystem`
- **主映射：** Ch04（RAII）· **辅映射：** Ch07（标准库精讲）

### DirScanner — 目录扫描与分析工具
- **路径：** `src/DirScanner/`
- **难度：** ⚡ 初级 · **教程：** `src/DirScanner/TUTORIAL.md`
- **知识点：** `std::filesystem` `递归遍历` `优先队列` `Top-K算法` `JSON输出` `STL算法`
- **主映射：** Ch07（标准库精讲）· **辅映射：** Ch28（文件系统接口）

### IniParser — INI 配置文件解析器
- **路径：** `project/IniParser/`
- **难度：** ⚡ 初级 · **视频：** 12 期 · **教程：** `project/IniParser/tutorial/`
- **知识点：** `string_view` `optional` `字符串处理` `CMake工程组织`
- **主映射：** Ch21（手写配置系统）· **辅映射：** Ch07（标准库精讲）

### MemoryPool — 高性能内存池
- **路径：** `project/memory_pool/`
- **难度：** 🔥 中级 · **视频：** 9 期
- **知识点：** `内存管理` `free-list` `thread-local cache` `性能优化` `benchmark`
- **主映射：** Ch08（并发基础）· **辅映射：** Ch32（并发进阶）

### Mimalloc — 微软开源分配器源码阅读
- **路径：** `project/external/mimalloc/`
- **难度：** 💎 进阶 · **视频：** 5 期
- **知识点：** `开源项目阅读` `高级内存分配器` `分段设计` `线程缓存`
- **映射：** Ch32（并发进阶）— 进阶补充材料

---

## 计划中项目清单

> 以下项目均已纳入课程规划，将随对应章节逐步落地。

### JSON Parser — JSON 序列化解析器
- **难度：** ⚡ 初级 · **映射章节：** Ch38（序列化框架设计）
- **知识点：** `递归下降解析` `variant/visit` `Unicode处理` `AST构建`
- **项目定位：** 独立项目，sysmon 监控数据 JSON 输出模块

### mini HTTP Server — HTTP/1.1 服务器
- **难度：** 🔥 中级 · **映射章节：** Ch37（HTTP 协议与服务器）
- **知识点：** `Socket编程` `HTTP协议` `epoll` `请求路由` `MIME类型`
- **项目定位：** 独立项目，sysmon HTTP 上报接口

### TinyRPC — 最简 RPC 框架
- **难度：** 🔥 中级 · **映射章节：** Ch40（RPC 框架入门）
- **知识点：** `序列化+网络+分发` `服务注册` `调用语义` `错误传播`
- **项目定位：** 独立项目，sysmon 远程查询接口

### Coroutine Task — 异步任务库
- **难度：** 🔥 中级 · **映射章节：** Ch43-44（C++20 协程）
- **知识点：** `协程帧` `co_await` `promise_type` `异步任务链` `调度器`
- **项目定位：** 独立项目，sysmon 异步采集引擎

### Binary Codec — 二进制序列化工具
- **难度：** ⚡ 初级 · **映射章节：** Ch39（二进制序列化）
- **知识点：** `字节序` `内存对齐` `版本兼容` `zero-copy`
- **项目定位：** 独立项目，sysmon 高效采集数据格式

### TUI Dashboard — 终端可视化面板
- **难度：** 🔥 中级 · **映射章节：** Ch60（输出层与终端 UI）
- **知识点：** `ANSI escape` `FTXUI/ncurses` `实时刷新` `布局系统`
- **项目定位：** sysmon 终端实时可视化

### Lock-free Stack — 无锁数据结构
- **难度：** 💎 进阶 · **映射章节：** Ch33（内存模型与原子操作）
- **知识点：** `CAS操作` `内存序` `ABA问题` `hazard pointer`
- **项目定位：** 独立技能分支

### SIMD Lab — 向量化加速实验
- **难度：** 🔥 中级 · **映射章节：** Ch35（SIMD 与向量化）
- **知识点：** `SSE/AVX/NEON` `SoA/AoS` `自动向量化` `性能benchmark`
- **项目定位：** 独立技能分支

### pybind11 Binding — Python 绑定
- **难度：** ⚡ 初级 · **映射章节：** Ch54（FFI 与语言互操作）
- **知识点：** `pybind11` `C ABI` `引用计数` `类型映射`
- **项目定位：** sysmon Python 绑定

---

## sysmon 贯穿项目时间线

> sysmon 是一个 Linux 系统监控工具，贯穿全部 62 章逐步构建，是课程的核心产出。

### Phase 0：基础搭建（Ch00-02）
- 创建仓库、CMake 多目标骨架
- **状态：** 未开始

### Phase 1：语言能力集成（Ch03-12）
- move 语义传递数据包、RAII 封装 /proc 读取
- 智能指针管理模块、filesystem 工具、并发采样线程
- 错误处理体系、类型擦除插件接口
- **状态：** 未开始

### Phase 2：工程基础设施（Ch13-25）
- CI/CD 流水线、Sanitizer/代码质量检查
- 包管理（vcpkg/Conan）、Docker 构建
- 分层架构重构、日志/配置/线程池/CLI 四大模块
- gtest 测试套件、Pimpl API 稳定化
- **状态：** 未开始

### Phase 3：系统核心实现（Ch26-35）
- 理解编译链接，符号导出规范化
- 插件化架构（dlopen 动态加载采集模块）
- 文件监听热更新、CPU/内存/网络/磁盘四路采集
- epoll 事件循环、timerfd 定时采样
- 并发审查、内存序优化、性能 profiling
- SIMD 加速数据聚合
- **状态：** 未开始

### Phase 4：网络扩展（Ch36-41）
- HTTP 上报接口、JSON 监控数据输出
- 二进制高效采集格式、RPC 远程查询
- io_uring 高性能采集后端
- **状态：** 未开始

### Phase 5：高级重构（Ch42-49）
- 类型特征工具集、设计模式审查
- 协程异步采集引擎、PMR 采集缓冲区
- 可测试化架构重构
- **状态：** 未开始

### Phase 6：生产化（Ch50-57）
- 公共 API 稳定化、代码规范落地
- 安全审计、Fuzzing 测试、Python 绑定
- 结构化日志、自监控系统
- **状态：** 未开始

### Phase 7：整合发布（Ch58-62）
- 架构文档、主引擎组装、终端 UI Dashboard
- CI/CD 自动打包发布
- 完整 README、作品集就绪
- **状态：** 未开始

---

## 进阶方向

完成全部课程后，可根据兴趣选择以下方向深入：

**🔧 系统方向**
内核模块 → 文件系统驱动 → 容器运行时 → 数据库引擎

**🌐 网络方向**
HTTP Server → RPC 框架 → 分布式 KV 存储 → 消息队列

**🎮 游戏方向**
2D 引擎 → ECS 架构 → 物理引擎 → 渲染管线

**🤖 AI 方向**
矩阵库 → 神经网络推理 → ONNX Runtime → CUDA 加速

**推荐开源项目阅读：**
- **系统级：** [muduo](https://github.com/chenshuo/muduo)（网络库）、[brpc](https://github.com/apache/brpc)（RPC 框架）
- **工具类：** [MyTinySTL](https://github.com/Alinshans/MyTinySTL)（STL 实现）、[fmt](https://github.com/fmtlib/fmt)（格式化库）
- **框架级：** [LLVM](https://github.com/llvm/llvm-project)（编译器基础设施）、[ClickHouse](https://github.com/ClickHouse/ClickHouse)（列式数据库）

---

## 下一步行动

- [ ] 决定 sysmon 项目的仓库位置（独立仓库 vs 仓库内子目录）
- [ ] 从 Part 0（Ch00-02）或 Part 2（工程工具链）开始落地第一张教程
- [ ] DirScanner 完成集成（加入 git、更新 README、创建视频目录）
- [ ] 确定首批新项目的开发优先级（建议：JSON Parser → HTTP Server → Coroutine Task）
