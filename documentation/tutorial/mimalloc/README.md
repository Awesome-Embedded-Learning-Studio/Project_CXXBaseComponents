# Mimalloc 源码阅读教程

> **如何阅读开源项目：以 mimalloc 为例**

阅读优秀开源项目的源码是提升技术能力的绝佳方式。本教程将带领你阅读微软的 mimalloc 内存分配器源码，学习源码阅读的方法论，同时理解一个工业级内存分配器的设计与实现。

---

## 教程章节

| 章节 | 标题 | 内容概要 |
|------|------|----------|
| [00](00-approach.md) | 源码阅读方法 | 如何起手阅读一个开源项目 |
| [01](01-heap.md) | mi_heap_t 数据结构 | 从最近的 heap 入手 |
| [02](02-page.md) | mi_page_t 数据结构 | 理解页的管理方式 |
| [03](03-malloc.md) | mi_malloc 实现 | 分配流程的详细分析 |
| [04](04-free.md) | mi_free 实现 | 释放流程的详细分析 |

---

## 配套资源

- **官方仓库**: [microsoft/mimalloc](https://github.com/microsoft/mimalloc)
- **论文**: [mimalloc: Free List Shuffling, Caching, Partial Coalescing, and Segregation](https://www.microsoft.com/en-us/research/publication/mimalloc-free-list-shuffling-caching-partial-coalescing-and-segregation/)
- **本地路径**: [project/external/mimalloc/](../../project/external/mimalloc/)
- **视频教程**: [B站 - mimalloc 源码阅读](https://space.bilibili.com/294645890/lists/7045956)（5集）
- **作者**: [是的一个城管](https://space.bilibili.com/294645890)

---

## 你将学到什么

完成本教程后，你将：

- 掌握阅读大型开源项目的正确方法
- 理解工业级内存分配器的设计原理
- 了解 mimalloc 的核心优化技巧
- 学会从论文和代码中学习系统设计
- 能够独立阅读其他复杂开源项目
