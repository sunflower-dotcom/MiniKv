# MiniKV — LSM-Tree 存储引擎

从零实现的嵌入式 Key-Value 存储引擎（LevelDB 简化版），用于学习和理解 LSM-Tree 核心原理。

## 架构

```
写入请求
   │
   ▼
┌─────────┐   flush    ┌──────────┐
│ MemTable │ ────────► │ SSTable 0 │
│ (跳表)    │           │ SSTable 1 │
└─────────┘           │   ...     │
   │                   └──────────┘
   │ 写前日志                │
   ▼                        ▼
┌─────────┐           ┌─────────────┐
│   WAL   │           │ Compaction  │
│ (恢复用) │           │   (归并)    │
└─────────┘           └─────────────┘
```

## 阶段路线

| 阶段 | 模块 | 状态 |
|------|------|------|
| 1 | 项目骨架、公共接口 | ✅ 完成 |
| 2 | MemTable（跳表实现） | ✅ 完成 |
| 3 | WAL 预写日志 | ⬜ 待开始 |
| 4 | SSTable 磁盘文件 | ⬜ 待开始 |
| 5 | Compaction 归并合并 | ⬜ 待开始 |
| 6 | Bloom Filter | ⬜ 待开始 |
| 7 | Benchmark 性能测试 | ⬜ 待开始 |

## MemTable — 跳表实现

`src/memtable/skiplist.h` — 概率性多层链表，提供 O(log n) 平均读写性能。

```
Level 3:  head ──────────────────────► 50 ──► NULL
Level 2:  head ──────► 30 ──────────► 50 ──► NULL
Level 1:  head ──► 10 ──► 30 ──► 40 ──► 50 ──► NULL
Level 0:  head ──► 10 ──► 30 ──► 40 ──► 50 ──► NULL
```

### 数据结构

| 组件 | 说明 |
|------|------|
| `Node` | 存储 `key`（`std::string`）、`value`（`std::string`）、`next[]`（多级前向指针数组） |
| `head_` | 哨兵节点，持有 `kMaxLevel` 个指针，key/value 为空 |
| `level_` | 当前实际使用的最高层数（1 ~ kMaxLevel） |
| `size_bytes_` | 所有 key+value 的字节数累计，用于判断 flush 阈值 |

### 关键参数

| 参数 | 值 | 说明 |
|------|-----|------|
| `kMaxLevel` | 16 | 最大层数，支持约 65536 条目 |
| `kProbability` | 0.5 | 每次晋升下一层的概率（几何分布） |

### 核心操作

**put(key, value) — O(log n)**

```
1. 从 head_ 最高层出发，向右移动到 key 的位置
2. 每层记录前驱节点到 update[] 数组
3. 到达 level 0 后检查 key 是否已存在：
   - 已存在 → 更新 value，调整 size_bytes_
   - 不存在 → random_level() 生成新节点层数，在各层插入
```

**get(key) — O(log n)**

```
1. 从最高层向右移动，跳过所有 key < 目标 key 的节点
2. 到达 level 0 后检查 cur->next[0] 是否匹配
3. 匹配返回 value，不匹配返回 nullopt
4. 使用 std::less<> 实现异质查找，避免 string_view → string 临时构造
```

**iter() — O(n)**

```
1. 沿 level 0 链表顺序遍历所有节点
2. 过滤 value == "__TOMBSTONE__" 的墓碑记录
3. 返回排序后的非删除键值对 vector
```

**clear() — O(n)**

```
1. 遍历 level 0 链表释放所有节点
2. 重置 head_ 的所有指针为 nullptr
3. 重置 level_ = 1, size_bytes_ = 0
```

### 随机层数生成

使用 Mersenne Twister (`std::mt19937`) 生成几何分布：每层有 50% 概率晋升到下一层，直到达到 `kMaxLevel` 或概率判定停止。种子由 `std::random_device` 提供。

### 线程安全

跳表自身非线程安全，由 `MemTable::Impl` 中的 `std::mutex` 统一加锁。所有公共方法进入时获取 `lock_guard`。

### 墓碑机制

`del(key)` 调用 `put(key, "__TOMBSTONE__")`，将删除标记作为普通值存入跳表。`get()` 对已删除 key 返回 `"__TOMBSTONE__"`；`iter()` 自动过滤墓碑记录。

### 物理文件布局

```
include/minikv/memtable/memtable.h  ← 公共接口（PIMPL）
src/memtable/memtable.cpp           ← MemTable::Impl（持有 SkipList）
src/memtable/skiplist.h             ← 跳表实现（header-only）
```

## 构建

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## 运行

```bash
# 交互式 CLI
./build/minikv-cli
```

## 测试

```bash
cd build && ctest
# 或直接运行
./build/minikv-tests
```
