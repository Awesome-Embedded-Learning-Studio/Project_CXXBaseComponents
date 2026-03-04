# 01 - FreeList 设计

## 内存池的基石

在构建复杂的内存池之前，我们需要先理解最基础的组件：FreeList（自由链表）。这是一个简单但强大的数据结构，用于管理空闲的内存块。

### 什么是 FreeList

FreeList 的核心思想是：**把所有空闲的内存块串成一个链表，需要的时候从链表头部取一个，释放的时候放回链表**。

想象一下，你有一堆相同大小的积木。你需要积木的时候，从堆顶拿一块；用完了，放回堆顶。这就是 FreeList 的本质。

### 数据结构设计

FreeList 只需要两个东西：

1. 一个节点结构，指向下一个节点
2. 一个头指针，指向链表的第一个节点

```cpp
struct FreeNode {
  FreeNode *next;
};
```

注意 `FreeNode` 只有一个指针字段。这是因为我们只需要链表功能，不需要存储其他信息。当内存块被"分配"出去时，它就不再是 `FreeNode`，而是用户的数据。当它被"释放"回来时，它又变成 `FreeNode`。

这种设计非常巧妙：**同一个内存块，在不同时期扮演不同的角色**。从系统分配来的内存，我们可以把它强制转换成 `FreeNode*` 来管理。

### FreeList 类的实现

```cpp
class FreeList {
public:
  // 把一个节点推入链表头部
  void push(FreeNode *node) {
    node->next = head;
    head = node;
  }

  // 从链表头部弹出一个节点
  FreeNode *pop() {
    if (!head) {
      return nullptr;
    }

    FreeNode *node = head;
    head = node->next;
    return node;
  }

  // 检查链表是否为空
  bool empty() const { return head == nullptr; }

private:
  FreeNode *head = nullptr;
};
```

这个实现非常简洁，所有操作都是 O(1) 的时间复杂度。

### push 操作图解

```
初始状态：
head ──→ nullptr

push(node1):
node1.next = head (nullptr)
head = node1

head ──→ node1 ──→ nullptr

push(node2):
node2.next = head (node1)
head = node2

head ──→ node2 ──→ node1 ──→ nullptr
```

注意新节点总是被推到头部，这样操作最快——只需要改两个指针。

### pop 操作图解

```
当前状态：
head ──→ node2 ──→ node1 ──→ nullptr

pop():
node = head (node2)
head = node.next (node1)
return node

返回 node2，链表变成：
head ──→ node1 ──→ nullptr
```

同样，只需要改一个指针。

### 为什么用单向链表

你可能会问：为什么不用双向链表？双向链表可以支持 O(1) 的删除操作，而单向链表删除一个节点需要先找到前驱。

答案是：**我们不需要删除中间节点**。在内存池的使用场景中，我们只关心从头部取和放回头部，不需要删除任意位置的节点。单向链表更简单，每个节点少存一个指针，节省内存。

### 内存复用的技巧

FreeList 的一个关键技巧是：**内存块在"空闲"和"使用"两种状态间切换**。

当一块内存空闲时：

```
┌─────────────┐
│ FreeNode    │
│ next: ──────┼──→ next node
└─────────────┘
```

当这块内存被分配给用户时：

```
┌─────────────┐
│ User Data   │
│ (可以是任何类型)
└─────────────┘
```

同一个内存位置，根据状态不同被解释成不同的结构。这就是 C/C++ 的强大之处——我们可以自由地解释内存。

```cpp
// 分配内存
void* mem = ::operator new(size);
FreeNode* node = static_cast<FreeNode*>(mem);
// 现在 node 可以被放入 FreeList

// 从 FreeList 取出
FreeNode* node = freelist.pop();
void* user_mem = static_cast<void*>(node);
// 现在 user_mem 可以给用户使用
```

### 对齐问题

在实际使用中，我们需要考虑内存对齐。大多数架构要求指针类型按其对齐值对齐（通常是 8 字节）。如果我们用 8 字节对齐，那么 `FreeNode` 的大小至少是 8 字节（一个指针）。

```cpp
static constexpr size_t kClassGrid = 16;  // 16 字节对齐
```

16 字节对齐的好处是：它既能满足指针对齐，又能满足大多数类型的对齐要求（比如 `double` 是 8 字节对齐，`long double` 是 16 字节对齐）。

### 空链表的处理

当 FreeList 为空时，`pop()` 返回 `nullptr`。调用者需要检查返回值：

```cpp
FreeNode* node = freelist.pop();
if (!node) {
  // 链表空了，需要从中心池或系统获取更多内存
  node = fetch_from_central_pool();
}
```

在我们的内存池设计中，ThreadCache 的 FreeList 为空时，会从 CentralPool 批量获取多个节点，填满本地缓存。这样下次分配时就不需要频繁访问 CentralPool 了。

### 一个完整的使用示例

让我们看一个简单的使用示例：

```cpp
#include <iostream>
#include "memory_pool.hpp"

int main() {
    FreeList freelist;

    // 分配一些内存块并推入 FreeList
    for (int i = 0; i < 5; ++i) {
        void* mem = ::operator new(16);  // 分配 16 字节
        FreeNode* node = static_cast<FreeNode*>(mem);
        freelist.push(node);
        std::cout << "Pushed node " << i << "\n";
    }

    // 从 FreeList 弹出内存块
    while (!freelist.empty()) {
        FreeNode* node = freelist.pop();
        std::cout << "Popped node\n";
        ::operator delete(node);  // 归还给系统
    }

    return 0;
}
```

输出：

```
Pushed node 0
Pushed node 1
Pushed node 2
Pushed node 3
Pushed node 4
Popped node
Popped node
Popped node
Popped node
Popped node
```

### 小结

FreeList 是内存池的基石，它简单但高效：

1. **O(1) 操作**：push 和 pop 都是常数时间
2. **缓存友好**：只操作链表头部，访问模式可预测
3. **低开销**：每个空闲块只需要一个指针的额外开销
4. **灵活复用**：同一块内存可以在空闲和使用两种状态间切换

但 FreeList 本身还不足以构成一个完整的内存池。它只管理大小相同的内存块。下一章我们介绍 CentralPool，它管理多个不同大小的 FreeList，实现 Size Classes 的设计。

[下一章：CentralPool 实现 →](02-central.md)
