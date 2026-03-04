# 📺 Mimalloc 源码阅读

> 如何阅读开源项目：以 mimalloc 为例
>
> 外部仓库：[microsoft/mimalloc](https://github.com/microsoft/mimalloc)
>
> 本仓库路径：[project/external/mimalloc](../project/external/mimalloc/)

---

## 📚 视频目录

### 🔍 源码阅读实战

| # | 视频标题 | B站链接 | 状态 |
|---|----------|---------|------|
| 1 | 如何起手阅读一个开源项目？（mimalloc-1） | [📺](https://www.bilibili.com/video/BV1mzFwzeE3D/) | ✅ |
| 2 | 从最近的heap入手（mimalloc-2） | [📺](https://www.bilibili.com/video/BV1tBFwzNENX/) | ✅ |
| 3 | 第二个数据结构-mi_page_t（mimalloc-3） | [📺](https://www.bilibili.com/video/BV1xkFwzaEav/) | ✅ |
| 4 | mi_malloc的实现细节（mimalloc-4） | [📺](https://www.bilibili.com/video/BV12rFwzAEkB/) | ✅ |
| 5 | mi_free的实现细节（mimalloc-5） | [📺](https://www.bilibili.com/video/BV18YFwzBEzx/) | ✅ |

---

## 📝 学习路线

```
┌─────────────────────────────────────────────────────────┐
│                  Mimalloc 源码阅读路线                   │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Step 1: 阅读方法论                                      │
│  ├── 如何起手阅读一个开源项目                           │
│  └── 学习源码阅读的正确姿势                             │
│                                                         │
│  Step 2: 核心数据结构                                    │
│  ├── 从最近的heap入手                                   │
│  └── 第二个数据结构-mi_page_t                           │
│                                                         │
│  Step 3: 内存操作实现                                    │
│  ├── mi_malloc的实现细节                                │
│  └── mi_free的实现细节                                  │
│                                                         │
│  Step 4: 总结与扩展                                      │
│  └── 尝试阅读其他开源项目                               │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

## 🔗 相关链接

- **官方仓库**: [microsoft/mimalloc](https://github.com/microsoft/mimalloc)
- **本地路径**: [project/external/mimalloc](../project/external/mimalloc/)
- **论文**: [mimalloc: Free List Shuffling, Caching, Partial Coalescing, and Segregation](https://www.microsoft.com/en-us/research/publication/mimalloc-free-list-shuffling-caching-partial-coalescing-and-segregation/)
- **UP主**: [是的一个城管](https://space.bilibili.com/294645890)
