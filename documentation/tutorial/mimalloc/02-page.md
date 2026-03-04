# 02 - mi_page_t 数据结构

## 内存页的管理方式

理解了 heap 之后，我们深入到页（page）的层面。`mi_page_t` 是 mimalloc 管理内存页的核心结构。

### 页的概念

mimalloc 从操作系统申请内存时，以"页"为单位。一页通常是 4KB 或更大（取决于系统配置）。每个页被切分成固定大小的块，用于分配给用户。

### mi_page_t 的定义

```c
struct mi_page_s {
  mi_heap_t* heap;           // 所属的 heap
  mi_segment_t* segment;     // 所属的段
  size_t block_size;         // 每个块的大小
  size_t capacity;           // 能容纳多少个块
  size_t used;               // 已使用多少个块
  mi_block_t* free;          // 自由链表头
  size_t free_count;         // 自由块数量
  bool is_zero_init;         // 是否需要零初始化
  ...
};
```

### 核心字段详解

**block_size**：这个页里每个块的大小。同一个页内的块大小相同，这是 Size Classes 设计的体现。

**capacity**：这个页最多能容纳多少个块。计算方式是 `页大小 / block_size`。

**used**：已分配的块数量。当 `used == capacity` 时，页满了。

**free**：指向自由链表的头指针。这是一个单向链表，串起所有可用的块。

**free_count**：自由块的数量，用于快速判断页是否有空闲空间。

### 自由链表的实现

自由链表是页管理的核心：

```
Page:
┌──────────────────────────────────────┐
│ capacity: 100                         │
│ used: 60                              │
│ free: ───────────────────────────┐    │
└───────────────────────────────────┼────┘
                                    ↓
Block 1 ──→ Block 2 ──→ Block 3 ──→ ... ──→ nullptr
```

每个块的第一个 8 字节（或指针大小）存储指向下一个块的指针。当块被分配给用户时，这 8 字节就被用户数据覆盖了；当块被释放时，又重新填入下一个块的地址。

这种设计非常巧妙：内存块在"使用"和"空闲"两种状态间切换，同一块内存扮演不同的角色。

### 页的查找

当分配请求到来时，mimalloc 需要找到一个有足够空间且块大小匹配的页：

```c
mi_page_t* mi_find_page(mi_heap_t* heap, size_t size) {
    size_t bin = _mi_bin(size);
    mi_page_t* page = heap->pages[bin];
    if (page && page->free_count > 0) {
        return page;  // 找到了
    }
    // 没找到，需要从 segment 获取新页
    return mi_page_fresh(heap, size);
}
```

### 页的回收

当一个页的所有块都被释放后，页可以被回收：

```c
void mi_page_retire(mi_page_t* page) {
    if (page->used == 0) {
        // 所有块都释放了，可以归还给系统
        mi_segment_page_free(page);
    }
}
```

但 mimalloc 不会立即归还，而是把页放在 `page_free` 链表上，延迟回收。这样可以应对"释放后立即再分配"的模式。

### 小结

`mi_page_t` 是 mimalloc 内存管理的中间层：

1. **承上启下**：上接 heap，下接具体内存块
2. **自由链表**：高效管理空闲块
3. **统计信息**：used/free_count 帮助快速决策
4. **延迟回收**：不立即归还系统，提高重用率

下一章我们看 `mi_malloc` 的完整实现，理解分配流程是如何串起这些组件的。

[下一章：mi_malloc 实现 →](03-malloc.md)
