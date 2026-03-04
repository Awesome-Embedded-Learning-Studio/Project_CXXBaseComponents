# 03 - ThreadLocal 缓存

## 消除锁竞争

CentralPool 虽然能管理多个 Size Class，但它的所有操作都需要加锁。在高并发场景下，锁竞争会变成严重的瓶颈。ThreadCache（线程本地缓存）通过让每个线程拥有独立的内存缓存，大幅减少锁竞争。

### thread_local 的魔法

C++11 引入了 `thread_local` 关键字，用来声明线程局部存储（Thread Local Storage, TLS）。每个线程都有自己独立的副本，互不干扰：

```cpp
thread_local int counter = 0;

void func() {
    counter++;  // 每个线程有自己的 counter
    std::cout << counter << "\n";
}

int main() {
    std::thread t1(func);  // 输出 1
    std::thread t2(func);  // 输出 1（不同的 counter！）
    t1.join();
    t2.join();
    return 0;
}
```

`thread_local` 变量的生命周期随线程：线程创建时初始化，线程结束时销毁。

### ThreadCache 的设计

ThreadCache 的设计思路是：

1. 每个线程有自己的 FreeList 数组（一个 Size Class 一个）
2. 分配时先从本地缓存取，取不到再从 CentralPool 批量获取
3. 释放时直接放回本地缓存
4. 本地缓存足够大时，可以归还一部分给 CentralPool

```cpp
class ThreadCache {
public:
  // 从线程本地缓存分配内存
  FreeNode *alloc(size_t size, CentralPool &centralPool) {
    auto &list = freelists_[idx(size)];
    if (!list.empty()) {
      return list.pop();  // 本地有缓存，直接返回
    }

    // 本地缓存空了，从 CentralPool 批量获取
    for (size_t i = 0; i < kFetchTime; i++) {
      FreeNode* node = centralPool.fetch(size);
      if (node) {
        list.push(node);
      } else {
        // CentralPool 也没有，从系统分配
        list.push(systemNewBlock(size));
      }
    }

    return list.pop();  // 现在肯定有了
  }

  // 释放内存到线程本地缓存
  void free(size_t size, FreeNode *node) {
    freelists_[idx(size)].push(node);
  }

private:
  static constexpr size_t kMaxSmallSize = _kMaxSmallSize;
  static constexpr size_t kClassGrid = _kClassGrid;
  static constexpr size_t kNumClasses = kMaxSmallSize / kClassGrid;
  static constexpr size_t kFetchTime = 32;  // 每次批量获取 32 个块

  FreeList freelists_[kNumClasses];
  // 注意：这里不需要 mutex，因为每个线程独立

  static size_t idx(size_t sz) {
    size_t aligned = (sz + kClassGrid - 1) / kClassGrid;
    if (aligned == 0) {
      aligned = 1;
    }
    return aligned - 1;
  }

  // 从系统分配一个块
  static FreeNode *systemNewBlock(size_t size) {
    void *p = ::operator new(size);
    return reinterpret_cast<FreeNode *>(p);
  }
};
```

### 批量获取的优化

注意 `kFetchTime = 32`，这意味着当本地缓存为空时，我们从 CentralPool 批量获取 32 个块。这样做的好处是：

1. **减少锁竞争**：不是每次分配都访问 CentralPool，而是每 32 次才访问一次
2. **批量操作更高效**：获取 32 个块只需要加一次锁，而不是 32 次
3. **局部性好**：连续分配的块在物理内存上可能相邻，缓存命中率更高

### 分配流程图解

```
用户请求分配 32 字节
         ↓
ThreadCache.alloc(32)
         ↓
本地 FreeList[1] 是否为空？
         ↓
    否 → 直接返回
         ↓
    是 → 从 CentralPool 批量获取 32 个块
         ↓
        填满本地 FreeList[1]
         ↓
        返回一个块
```

### MemoryPool：整合三层结构

现在让我们把三层结构整合起来：

```cpp
class MemoryPool {
public:
  // 对齐辅助函数
  static inline constexpr size_t alignGrid(size_t n) {
    return (n + _kClassGrid - 1) & ~size_t(_kClassGrid - 1);
  }

  // 分配内存
  static void *alloc(size_t size) {
    size = alignGrid(size);  // 对齐

    // 大于 128 字节，直接走系统分配
    if (size > _kMaxSmallSize) {
      return ::operator new(size);
    }

    // 小对象分配，走线程本地缓存
    return tls_cache().alloc(size, central_pool);
  }

  // 释放内存
  static void free(void *ptr, size_t size) {
    if (!ptr) {
      return;
    }

    size = alignGrid(size);

    // 大对象，直接走系统释放
    if (size > _kMaxSmallSize) {
      ::operator delete(ptr);
      return;
    }

    // 小对象，放回线程本地缓存
    tls_cache().free(size, reinterpret_cast<FreeNode *>(ptr));
  }

  // 对象构造（placement new）
  template <typename T, typename... Args>
  static T *make(Args &&...args) {
    void *mem = alloc(sizeof(T));
    try {
      return new (mem) T(std::forward<Args>(args)...);
    } catch (...) {
      free(mem, sizeof(T));
      throw;
    }
  }

  // 对象析构
  template <typename T>
  static void destory(T *obj) {
    if (!obj)
      return;
    obj->~T();
    free(reinterpret_cast<void *>(obj), sizeof(T));
  }

private:
  // 获取线程本地缓存
  static ThreadCache &tls_cache() {
    thread_local ThreadCache cache;
    return cache;
  }

  static CentralPool central_pool;
};

// 定义静态成员
inline CentralPool MemoryPool::central_pool;
```

### thread_local 的陷阱

`thread_local` 虽然强大，但有几个需要注意的地方：

**第一，初始化开销**。`thread_local` 变量在第一次使用时初始化。如果初始化很复杂，第一次访问会有延迟：

```cpp
thread_local ThreadCache cache;  // 第一次访问时构造 ThreadCache
```

**第二，内存占用**。每个线程都有自己的副本，线程数量多时内存占用会增加。但 ThreadCache 很小（几个指针），通常不是问题。

**第三，动态线程**。程序运行期间创建的新线程也会有自己的副本。这对于我们是好事——新线程自动获得独立的缓存。

### 大对象的处理

我们只对小于等于 128 字节的对象使用内存池。大于 128 字节的对象直接走系统分配。这是因为：

1. **大对象相对较少**：大多数分配都是小对象（指针、小型结构体）
2. **缓存收益小**：大对象不适合频繁分配/释放的场景
3. **避免复杂化**：支持任意大小会增加很多复杂度

如果确实需要缓存大对象，可以考虑单独的大对象内存池。

### make() 和 destory() 的技巧

我们提供了 `make()` 和 `destory()` 辅助函数，方便直接构造/析构对象：

```cpp
// 传统方式
MyClass* obj = new MyClass();  // 使用系统的 new
delete obj;  // 使用系统的 delete

// 内存池方式
MyClass* obj = MemoryPool::make<MyClass>();  // 从内存池分配 + 构造
MemoryPool::destory(obj);  // 析构 + 归还内存池
```

`make()` 内部使用 placement new，在已分配的内存上构造对象。注意异常处理：如果构造函数抛异常，我们需要先释放内存，再重新抛异常。

### 小结

ThreadCache 通过 `thread_local` 实现线程本地缓存，大幅减少锁竞争：

1. **无锁快速路径**：大部分分配/释放不需要加锁
2. **批量操作**：定期与 CentralPool 交互，减少锁竞争次数
3. **自动管理**：`thread_local` 自动处理线程生命周期
4. **透明使用**：用户不需要关心线程本地细节

三层结构的内存池现在完整了：ThreadCache（无锁快速路径）→ CentralPool（共享缓存）→ System（系统分配）。下一章我们会进行性能测试，验证这个设计的效果。

[下一章：性能测试 →](04-benchmark.md)
