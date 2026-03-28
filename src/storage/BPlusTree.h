#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <string>
#include <map>
#include <functional>

#ifdef __linux__
#include "../server/concurrency/RWLock.h"
#endif

namespace minisql {
namespace storage {

// B+树节点
class BPlusTreeNode {
public:
    bool isLeaf;
    std::vector<int64_t> keys;
    std::vector<int> values;  // 叶子节点：row_id，内部节点：孩子指针
    std::vector<std::shared_ptr<BPlusTreeNode>> children;
    std::shared_ptr<BPlusTreeNode> next;  // 叶子节点链表指针

    BPlusTreeNode(bool leaf = true) : isLeaf(leaf) {}
};

// B+树
class BPlusTree {
public:
    BPlusTree(int order = 200);  // 阶数 m

    // 插入
    bool insert(int64_t key, int rowId);

    // 删除
    bool remove(int64_t key);

    // 查找
    int find(int64_t key);  // 返回 row_id，不存在返回 -1

    // 范围查询 [min, max]
    std::vector<int> rangeSearch(int64_t min, int64_t max);

    // 清空
    void clear();

    // 获取根节点（用于调试）
    std::shared_ptr<BPlusTreeNode> getRoot() { return root_; }

    // 序列化/反序列化
    std::string serialize() const;
    void deserialize(const std::string& data);

    // 获取阶数
    int getOrder() const { return order_; }

    // 获取节点数量
    int getNodeCount() const;

private:
    int order_;  // 阶数 m
    int minKeys_;  // 最少键数 ⌈m/2⌉ - 1
    std::shared_ptr<BPlusTreeNode> root_;

#ifdef __linux__
    mutable RWLock rwLock_;
#endif

    // 序列化辅助
    void serializeNode(const std::shared_ptr<BPlusTreeNode>& node, std::vector<std::string>& nodes, int parentIndex, int childIndex) const;

    // 反序列化辅助
    std::shared_ptr<BPlusTreeNode> insert(std::shared_ptr<BPlusTreeNode> node, int64_t key, int rowId, int64_t& splitKey);
    void splitChild(std::shared_ptr<BPlusTreeNode> parent, int index);
    std::shared_ptr<BPlusTreeNode> remove(std::shared_ptr<BPlusTreeNode> node, int64_t key);
    void borrowFromLeft(std::shared_ptr<BPlusTreeNode> node, int index);
    void borrowFromRight(std::shared_ptr<BPlusTreeNode> node, int index);
    void merge(std::shared_ptr<BPlusTreeNode> node, int index);
    int findKey(std::shared_ptr<BPlusTreeNode> node, int64_t key);
    int findChildIndex(std::shared_ptr<BPlusTreeNode> node, int64_t key);

    // 内部无锁查找（由外部调用者加锁）
    int findUnlocked(int64_t key);
    std::vector<int> rangeSearchUnlocked(int64_t min, int64_t max);
    bool insertUnlocked(int64_t key, int rowId);
    bool removeUnlocked(int64_t key);
};

}  // namespace storage
}  // namespace minisql
