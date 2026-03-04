# 04 - mi_free 实现

## 释放流程的详细分析

理解了分配流程，释放流程就相对简单了。`mi_free` 的核心是把块放回自由链表，同时维护页的统计信息。

### 入口函数

```c
void mi_free(void* p) {
    if (!p) return;  // free(NULL) 是合法的，什么也不做

    // 1. 根据地址找到对应的页
    mi_page_t* page = mi_page_of(p);

    // 2. 从页释放块
    mi_page_free(page, p);
}
```

### 地址到页的转换

`mi_page_of` 是个技巧性很强的函数。给定一个地址，它需要找到这个地址属于哪个页。

mimalloc 使用"段"的概念：一个段包含多个连续的页，每个段有自己的元数据。通过地址的某些位可以快速定位段和页。

```c
mi_page_t* mi_page_of(void* p) {
    // 1. 找到段（通过地址的高位）
    mi_segment_t* segment = mi_segment_of(p);

    // 2. 在段内找到页
    size_t page_index = mi_segment_page_index(segment, p);
    return &segment->pages[page_index];
}
```

具体实现涉及地址的位操作，这里不展开细节。关键是理解：通过指针可以反向找到管理它的页。

### 页的释放

`mi_page_free` 把块放回自由链表：

```c
void mi_page_free(mi_page_t* page, void* p) {
    // 1. 转换成 block 指针
    mi_block_t* block = (mi_block_t*)p;

    // 2. 推入自由链表
    block->next = page->free;
    page->free = block;

    // 3. 更新统计
    page->free_count++;
    page->used--;

    // 4. 如果页空了，考虑回收
    if (page->used == 0) {
        mi_page_retire(page);
    }
}
```

注意这里 `p` 直接被转换成 `mi_block_t*`，不需要加任何偏移。这是因为自由链表的指针"借用"了块的前 8 字节。

### 页的回收

`mi_page_retire` 决定是否真正回收页：

```c
void mi_page_retire(mi_page_t* page) {
    mi_heap_t* heap = page->heap;

    // 1. 放入 heap 的待回收链表
    page->next = heap->page_free;
    heap->page_free = page;

    // 2. 增加退休计数
    heap->page_retired++;

    // 3. 如果退休页太多，触发回收
    if (heap->page_retired > MI_RETIRE_MAX) {
        mi_heap_collect(heap);
    }
}
```

`MI_RETIRE_MAX` 是一个阈值，决定延迟多久才真正归还内存给系统。延迟回收的好处是：

1. **应对释放后立即分配的模式**
2. **减少系统调用的次数**
3. **批量操作更高效**

### 延迟回收的策略

mimalloc 不会立即把空页归还给系统，而是：

1. 先放入 `page_free` 链表
2. 当积累到一定数量时，批量归还
3. 如果内存紧张，会主动触发回收

这种策略在"频繁分配释放"的场景下特别有效。

### 完整流程图

```
mi_free(ptr)
         ↓
    ptr == NULL?
         ↓
    是 → 返回
         ↓
    否 → 找到对应的页
         ↓
    把块推入自由链表
         ↓
    更新统计信息
         ↓
    页空了？
         ↓
    是 → 标记为退休
         ↓
    退休页太多？
         ↓
    是 → 批量归还给系统
```

### 与 malloc 的配合

malloc 和 free 的配合非常高效：

1. **同一页内释放**：只需要几次指针操作
2. **无锁**：每个线程独立的页，不需要加锁
3. **延迟回收**：减少系统调用

### 性能对比

相比系统 malloc/free，mimalloc 的优势：

| 操作 | 系统 malloc | mimalloc |
|------|------------|----------|
| 小对象分配 | 需要锁 | 无锁（ThreadCache） |
| 小对象释放 | 需要锁+合并 | 无锁+延迟回收 |
| 大对象 | mmap | 直接 mmap |
| 多线程扩展性 | 线性下降 | 几乎线性 |

### 小结

`mi_free` 的实现简洁而高效：

1. **地址反向查找**：通过指针找到管理它的页
2. **O(1) 操作**：只需更新几个指针
3. **延迟回收**：不是立即归还系统
4. **批量处理**：积累到一定量再操作

### mimalloc 的核心技巧总结

阅读完 mimalloc 的源码，我们总结了它的核心优化技巧：

1. **Per-thread Heap**：线程本地堆，无锁访问
2. **Size Classes**：固定大小档位，减少碎片
3. **Free List Shuffling**：打乱自由链表，提高缓存利用率
4. **元数据复用**：借用用户空间存储指针
5. **延迟回收**：不立即归还系统，提高重用率

这些技巧不是孤立的，而是共同构成了一个高效的内存分配器。

---

## 完结撒花

恭喜你完成 mimalloc 源码阅读教程！通过这次阅读，你不只理解了 mimalloc 的工作原理，更掌握了阅读大型开源项目的方法论。

源码阅读不是一蹴而就的，需要反复、深入。第一次读可能只理解框架，第二次读能看懂细节，第三次读能欣赏设计巧思。

去读更多优秀的开源项目吧——Linux 内核、Redis、LLVM...每一个都是宝库，等待你去挖掘。

[← 返回教程首页](README.md)
