# 03 - mi_malloc 实现

## 分配流程的详细分析

现在我们理解了 `mi_heap_t` 和 `mi_page_t`，是时候看完整的分配流程了。`mi_malloc` 是 mimalloc 对外暴露的主要接口，它的实现展示了所有组件如何协同工作。

### 入口函数

```c
void* mi_malloc(size_t size) {
    // 1. 对齐大小
    size = _mi_wsize(size);

    // 2. 获取当前线程的 heap
    mi_heap_t* heap = mi_heap_get_default();

    // 3. 从 heap 分配
    return mi_heap_malloc(heap, size);
}
```

入口很简洁，主要做三件事：对齐、获取 heap、调用 heap_malloc。

### 对齐操作

`_mi_wsize` 对请求的大小进行对齐：

```c
static inline size_t _mi_wsize(size_t size) {
    if (size < 16) return 16;  // 最小 16 字节
    return (size + 15) & ~15;  // 向上对齐到 16 字节
}
```

对齐有两个目的：

1. **满足对齐要求**：大多数类型需要 8 或 16 字节对齐
2. **减少档位数量**：不用为每个字节大小都维护一个档位

### heap_malloc 的实现

```c
void* mi_heap_malloc(mi_heap_t* heap, size_t size) {
    // 1. 找到合适的页
    mi_page_t* page = mi_page_find(heap, size);
    if (!page) {
        // 没找到，申请新页
        page = mi_page_fresh(heap, size);
    }

    // 2. 从页分配块
    return mi_page_malloc(page, size);
}
```

### 页的查找

`mi_page_find` 在 heap 的页数组中搜索：

```c
mi_page_t* mi_page_find(mi_heap_t* heap, size_t size) {
    size_t bin = _mi_bin(size);  // 计算档位索引
    mi_page_t* page = heap->pages[bin];

    // 检查页是否可用
    if (page && page->free_count > 0) {
        return page;
    }

    return NULL;  // 需要新页
}
```

### 从页分配块

`mi_page_malloc` 是核心：

```c
void* mi_page_malloc(mi_page_t* page, size_t size) {
    // 1. 从自由链表取一个块
    mi_block_t* block = page->free;

    // 2. 更新自由链表
    page->free = block->next;
    page->free_count--;
    page->used++;

    // 3. 返回块的内存（跳过元数据）
    return _mi_page_ptr(block);
}
```

`_mi_page_ptr` 把 `mi_block_t*` 转换成 `void*`：

```c
static inline void* _mi_page_ptr(mi_block_t* block) {
    // 块的地址 + sizeof(mi_block_t)
    return (void*)((char*)block + sizeof(mi_block_t));
}
```

等等，这里有个细节：`sizeof(mi_block_t)` 是多少？

```c
struct mi_block_s {
    mi_block_t* next;  // 只有一个指针！
};
```

所以 `sizeof(mi_block_t) == sizeof(void*)`，通常是 8 字节（64 位系统）。

但这里有个问题：用户请求 16 字节，我们实际分配的是 `16 + 8 = 24` 字节？

答案是：**不是**。mimalloc 的自由链表指针"借用"了块的前 8 字节。当块被分配时，这 8 字节就成了用户数据的一部分；当块被释放时，这 8 字节用来存储下一个块的指针。

这就是"元数据复用"的技巧——没有额外的元数据开销！

### 新页的分配

当找不到合适的页时，需要申请新页：

```c
mi_page_t* mi_page_fresh(mi_heap_t* heap, size_t size) {
    // 1. 从 segment 分配页
    mi_page_t* page = mi_segment_page_alloc(heap, size);

    // 2. 初始化自由链表
    mi_page_init_free(page, size);

    // 3. 放入 heap 的页数组
    size_t bin = _mi_bin(size);
    heap->pages[bin] = page;

    return page;
}
```

初始化自由链表：

```c
void mi_page_init_free(mi_page_t* page, size_t size) {
    char* start = page->start;  // 页的起始地址
    size_t count = page->capacity;

    // 把所有块串成链表
    for (size_t i = 0; i < count; ++i) {
        mi_block_t* block = (mi_block_t*)(start + i * size);
        block->next = (i + 1 < count) ?
            (mi_block_t*)(start + (i + 1) * size) : NULL;
    }

    page->free = (mi_block_t*)start;
    page->free_count = count;
}
```

### 完整流程图

```
mi_malloc(size)
         ↓
    对齐 size
         ↓
获取 thread heap
         ↓
计算 bin index
         ↓
heap->pages[bin] 有页且有空块？
         ↓
    是 → 返回块
         ↓
    否 → 申请新页
         ↓
    初始化自由链表
         ↓
    返回块
```

### 小结

`mi_malloc` 的实现展示了 mimalloc 的核心优化：

1. **线程本地**：每个线程独立的 heap，无锁访问
2. **Size Classes**：按大小分类，减少碎片
3. **元数据复用**：自由链表指针借用块空间
4. **快速路径**：大多数情况只需几次指针操作

下一章我们看 `mi_free`，理解释放流程如何高效地回收内存。

[下一章：mi_free 实现 →](04-free.md)
