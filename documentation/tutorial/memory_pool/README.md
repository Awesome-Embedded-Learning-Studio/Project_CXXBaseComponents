# MemoryPool 实战教程

> **从 malloc 到内存池：手写一个高性能分配器**

通用内存分配器（malloc/new）虽然方便，但它在高频分配/释放场景下性能并不理想。本教程将带你从零开始，实现一个高性能的内存池，理解现代分配器背后的设计原理。

---

## 教程章节

| 章节 | 标题 | 内容概要 |
|------|------|----------|
| [00](00-motivation.md) | 为什么需要内存池 | malloc 的性能瓶颈在哪里 |
| [01](01-freelist.md) | FreeList 设计 | 自由链表的基本原理 |
| [02](02-central.md) | CentralPool 实现 | 中心池的设计与线程安全 |
| [03](03-threadlocal.md) | ThreadLocal 缓存 | 线程本地缓存与批量分配 |
| [04](04-benchmark.md) | 性能测试 | Benchmark 对比与优化验证 |

---

## 配套资源

- **视频教程**: [B站 - 内存池实战](https://space.bilibili.com/294645890/lists/7045956)（9集）
- **源码位置**: [project/memory_pool/MyMemoryPool/](../../project/memory_pool/MyMemoryPool/)
- **相关项目**: [Project_MakeAMemroyPool](https://github.com/Awesome-Embedded-Learning-Studio/Project_MakeAMemroyPool)
- **作者**: [是的一个城管](https://space.bilibili.com/294645890)

---

## 你将学到什么

完成本教程后，你将：

- 理解通用内存分配器的性能瓶颈
- 掌握 Size Classes 和内存对齐的设计
- 理解自由链表（FreeList）的工作原理
- 掌握线程本地缓存（thread_local）的应用
- 了解中心池（CentralPool）的设计模式
- 能够编写符合 STL 标准的自定义分配器
