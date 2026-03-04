# 02 - CentralPool 实现

## 中心池的设计

FreeList 只能管理大小相同的内存块，但实际程序需要分配各种不同大小的内存。CentralPool（中心池）的作用就是管理多个不同大小的 FreeList，实现 Size Classes 的设计。

### Size Classes 回顾

我们之前讲过，将内存大小划分为固定的档位：

```cpp
static constexpr size_t kMaxSmallSize = 128;  // 最大支持 128 字节
static constexpr size_t kClassGrid = 16;      // 每 16 字节一档
static constexpr size_t kNumClasses = kMaxSmallSize / kClassGrid;  // 8 个档位
```

每个档位对应一个 FreeList：

| 档位索引 | 大小范围 | 实际分配 |
|---------|----------|----------|
| 0 | 1-16 | 16 |
| 1 | 17-32 | 32 |
| 2 | 33-48 | 48 |
| 3 | 49-64 | 64 |
| 4 | 65-80 | 80 |
| 5 | 81-96 | 96 |
| 6 | 97-112 | 112 |
| 7 | 113-128 | 128 |

### CentralPool 的数据结构

CentralPool 维护一个 FreeList 数组，每个元素对应一个 Size Class：

```cpp
class CentralPool {
public:
  // 从中心池获取一个内存块
  FreeNode *fetch(std::size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto &list = pool_[idx(size)];
    if (!list.empty()) {
      return list.pop();
    }
    return nullptr;  // 该档位没有空闲块
  }

  // 将内存块归还给中心池
  void release(size_t size, FreeNode *node) {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_[idx(size)].push(node);
  }

private:
  static constexpr size_t kMaxSmallSize = _kMaxSmallSize;
  static constexpr size_t kClassGrid = _kClassGrid;
  static constexpr size_t kNumClasses = kMaxSmallSize / kClassGrid;

  FreeList pool_[kNumClasses];  // 每个 Size Class 一个 FreeList
  std::mutex mutex_;             // 保护内部数据的锁

  // 计算大小对应的档位索引
  static size_t idx(size_t sz) {
    size_t aligned = (sz + kClassGrid - 1) / kClassGrid;
    if (aligned == 0) {
      aligned = 1;
    }
    return aligned - 1;
  }
};
```

### idx() 函数的原理

`idx(size)` 函数计算给定大小对应的档位索引。它本质上是一个向上取整的操作：

```cpp
// 假设 kClassGrid = 16
idx(1)   = (1 + 15) / 16 - 1 = 1 - 1 = 0   → 16 字节
idx(16)  = (16 + 15) / 16 - 1 = 1 - 1 = 0  → 16 字节
idx(17)  = (17 + 15) / 16 - 1 = 2 - 1 = 1  → 32 字节
idx(32)  = (32 + 15) / 16 - 1 = 2 - 1 = 1  → 32 字节
idx(128) = (128 + 15) / 16 - 1 = 8 - 1 = 7 → 128 字节
```

注意 `(sz + kClassGrid - 1) / kClassGrid` 是向上取整的常用技巧。对于正整数，`(a + b - 1) / b` 等价于 `ceil(a / b)`。

### 对齐的计算

注意 `idx()` 返回的是索引，实际分配的大小需要乘以 `kClassGrid`：

```cpp
static constexpr size_t alignGrid(size_t n) {
  return (n + kClassGrid - 1) & ~size_t(kClassGrid - 1);
}
```

这里用位运算代替除法，更高效。`~size_t(kClassGrid - 1)` 是把低 4 位清零（因为 16 = 2^4），所以 `n & ~15` 相当于把 n 向下对齐到 16 的倍数。

等等，这看起来是向下对齐？我们再仔细看：

```cpp
alignGrid(1)   = (1 + 15) & ~15 = 16 & ~16 = 16
alignGrid(16)  = (16 + 15) & ~15 = 31 & ~15 = 16
alignGrid(17)  = (17 + 15) & ~15 = 32 & ~15 = 32
alignGrid(128) = (128 + 15) & ~15 = 143 & ~15 = 128
```

原来是 `n + kClassGrid - 1` 先向上调整，然后再向下对齐。所以整体效果是向上对齐。

### 线程安全设计

CentralPool 是所有线程共享的，所以必须保证线程安全。我们用 `std::mutex` 保护所有操作：

```cpp
FreeNode *fetch(std::size_t size) {
  std::lock_guard<std::mutex> lock(mutex_);  // RAII 自动加锁/解锁
  auto &list = pool_[idx(size)];
  if (!list.empty()) {
    return list.pop();
  }
  return nullptr;
}
```

`std::lock_guard` 是 RAII 风格的锁管理：构造时加锁，析构时解锁。这确保了即使发生异常，锁也能被正确释放。

### 为什么 fetch() 返回 nullptr

你可能会注意到，当对应的 FreeList 为空时，`fetch()` 返回 `nullptr` 而不是自动分配新内存。这是有意的设计：

CentralPool 只负责管理已有的空闲内存块，不负责分配新内存。分配新内存的责任在 ThreadCache，它会：

1. 先尝试从 CentralPool 获取
2. 如果 CentralPool 为空，直接从系统分配
3. 分配多个块，填满本地缓存
4. 用不完的块可以归还给 CentralPool

这样设计的优点是职责分离：CentralPool 是简单的缓存管理器，ThreadCache 处理更复杂的策略。

### release() 操作

`release()` 操作比 `fetch()` 更简单：

```cpp
void release(size_t size, FreeNode *node) {
  std::lock_guard<std::mutex> lock(mutex_);
  pool_[idx(size)].push(node);
}
```

只需要把节点推入对应的 FreeList。不需要检查边界情况，因为 `push()` 总是成功的。

### 一个完整的使用示例

```cpp
#include <iostream>
#include <thread>
#include <vector>
#include "memory_pool.hpp"

void worker(CentralPool& pool, int id) {
    // 从中心池获取一些内存块
    std::vector<FreeNode*> nodes;
    for (int i = 0; i < 10; ++i) {
        FreeNode* node = pool.fetch(32);
        if (node) {
            nodes.push_back(node);
        }
    }

    std::cout << "Thread " << id << " fetched " << nodes.size() << " nodes\n";

    // 归还内存块
    for (auto* node : nodes) {
        pool.release(32, node);
    }
}

int main() {
    CentralPool pool;

    // 预先分配一些内存块
    for (int i = 0; i < 100; ++i) {
        void* mem = ::operator new(32);
        FreeNode* node = static_cast<FreeNode*>(mem);
        pool.release(32, node);
    }

    // 多线程测试
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker, std::ref(pool), i);
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
```

### 性能考虑

CentralPool 的所有操作都需要加锁，在高并发情况下可能成为瓶颈。但这正是下一章 ThreadCache 要解决的问题：

- ThreadCache 是线程本地的，大部分分配/释放不需要锁
- 只有当 ThreadCache 的本地缓存不足/溢出时，才访问 CentralPool
- 访问 CentralPool 时采用批量操作（一次取/放多个块），减少锁竞争

### 小结

CentralPool 是内存池的第二层，它管理多个不同大小的 FreeList：

1. **Size Classes**：将大小划分为固定的档位，减少碎片
2. **线程安全**：用 mutex 保护共享数据
3. **简单接口**：fetch() 和 release() 操作清晰明确
4. **职责分离**：只管理已有内存，不负责分配新内存

但 CentralPool 的锁竞争限制了并发性能。下一章我们引入 ThreadCache，通过线程本地缓存实现无锁的快速分配路径。

[下一章：ThreadLocal 缓存 →](03-threadlocal.md)
