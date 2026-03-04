# 01 - mi_heap_t 数据结构

## 从最近的 heap 入手

理解 mimalloc，最好的切入点是 `mi_heap_t`。这是每个线程独立的堆结构，也是分配内存的入口点。

### 为什么从 heap 开始

想象你是 mimalloc，现在来了一个 `mi_malloc(32)` 调用。你需要：

1. 找到当前线程的堆
2. 从堆里找合适的页
3. 从页里分配一个块

第一步就是"找到当前线程的堆"。`mi_heap_t` 就是这个"堆"。

### mi_heap_t 的定义

```c
// src/heap.h
struct mi_heap_s {
  mi_tld_t tld;              // 线程局部数据
  mi_page_t* pages[MI_BIN_FULL];  // 页的数组（按大小分类）
  mi_block_t* page_free;     // 可释放的页链表
  size_t page_count;         // 页的数量
  size_t page_retired;       // 已退休的页数量
  ...
};
```

让我们逐个字段理解：

**mi_tld_t tld**：线程局部数据（Thread Local Data），包含线程特有的信息。

**mi_page_t* pages[MI_BIN_FULL]**：这是一个数组，每个元素是一个页指针。数组按大小分类索引，类似我们之前讲的 Size Classes。

**mi_block_t* page_free**：这是一个页链表，存着可以归还给系统的页。

**page_count / page_retired**：统计信息，用于决定是否需要回收内存。

### 页数组的设计

`pages[MI_BIN_FULL]` 是核心。`MI_BIN_FULL` 是一个常量，定义了有多少个"桶"（bin）：

```c
#define MI_BIN_FULL 97
```

这意味着有 97 个桶，每个桶对应一个大小范围。请求分配时，根据大小计算桶索引：

```c
size_t bin = _mi_bin(size);
mi_page_t* page = heap->pages[bin];
```

### 线程局部存储

每个线程有自己的 heap，通过线程局部存储（TLS）实现：

```c
static inline mi_heap_t* mi_heap_get_default() {
    return _mi_heap_main_get();
}
```

在多线程环境下，每个线程访问自己的 heap，不需要加锁。这正是 mimalloc 高性能的关键之一。

### heap 的生命周期

heap 的生命周期和线程绑定：

1. 线程创建时，自动创建一个 heap
2. 线程退出时，heap 的资源被回收

这种设计让线程几乎不感知 heap 的存在——一切自动发生。

### 小结

`mi_heap_t` 是理解 mimalloc 的钥匙：

1. **线程隔离**：每个线程独立的 heap，无锁访问
2. **Size Classes**：pages 数组按大小分类
3. **自动管理**：生命周期和线程绑定
4. **扩展性**：可以添加更多统计和调优信息

下一章我们看 `mi_page_t`，理解单个页是如何管理的。

[下一章：mi_page_t 数据结构 →](02-page.md)
