#include "BPlusTree.h"
#include "../common/Logger.h"
#include <algorithm>

using namespace minisql;
using namespace storage;

BPlusTree::BPlusTree(int order) : order_(order) {
    minKeys_ = (order + 1) / 2;  // ceil(m/2)
    root_ = std::make_shared<BPlusTreeNode>(true);
}

bool BPlusTree::insert(int64_t key, int rowId) {
    if (find(key) != -1) {
        return false;  // 键已存在
    }

    int64_t splitKey = 0;
    auto newRoot = insert(root_, key, rowId, splitKey);

    if (newRoot) {
        auto oldRoot = root_;
        root_ = std::make_shared<BPlusTreeNode>(false);
        root_->keys.push_back(splitKey);
        root_->children.push_back(oldRoot);
        root_->children.push_back(newRoot);
    }

    return true;
}

std::shared_ptr<BPlusTreeNode> BPlusTree::insert(std::shared_ptr<BPlusTreeNode> node, int64_t key, int rowId, int64_t& splitKey) {
    if (node->isLeaf) {
        // 找到插入位置
        auto it = std::lower_bound(node->keys.begin(), node->keys.end(), key);
        int pos = it - node->keys.begin();

        node->keys.insert(it, key);
        node->values.insert(node->values.begin() + pos, rowId);

        // 检查是否需要分裂
        if (node->keys.size() > (size_t)order_) {
            auto newNode = std::make_shared<BPlusTreeNode>(true);
            size_t sz = node->keys.size();
            size_t mid = sz / 2;

            // 分裂点 key 上移到父节点
            // 左叶子保留 [0, mid)，不含 splitKey
            // 右叶子保留 [mid, sz)，包含 splitKey
            splitKey = node->keys[mid];

            // 先把 [mid, sz) 复制到新节点（右叶子包含 splitKey）
            for (size_t i = mid; i < sz; i++) {
                newNode->keys.push_back(node->keys[i]);
                newNode->values.push_back(node->values[i]);
            }

            // 左叶子截断到 [0, mid)
            node->keys.resize(mid);
            node->values.resize(mid);

            // 链接叶子节点
            newNode->next = node->next;
            node->next = newNode;

            return newNode;
        }
        return nullptr;
    }

    // 内部节点
    int index = findChildIndex(node, key);
    auto newChild = insert(node->children[index], key, rowId, splitKey);

    if (newChild) {
        // 插入分裂键和孩子
        node->keys.insert(node->keys.begin() + index, splitKey);
        node->children.insert(node->children.begin() + index + 1, newChild);

        // 检查是否需要分裂
        if (node->keys.size() > (size_t)order_) {
            auto newNode = std::make_shared<BPlusTreeNode>(false);
            size_t sz = node->keys.size();
            size_t mid = sz / 2;

            // 分裂点 key 上移到父节点
            splitKey = node->keys[mid];

            // 左节点保留 [0, mid)
            node->keys.resize(mid);

            // 右节点复制 [mid+1, sz)
            for (size_t i = mid + 1; i < sz; i++) {
                newNode->keys.push_back(node->keys[i]);
            }

            // 移动孩子到新节点
            size_t childCount = node->children.size();
            for (size_t i = mid + 1; i < childCount; i++) {
                newNode->children.push_back(node->children[i]);
            }
            node->children.resize(mid + 1);

            return newNode;
        }
    }

    return nullptr;
}

int BPlusTree::find(int64_t key) {
    auto node = root_;

    while (node) {
        if (node->isLeaf) {
            auto it = std::lower_bound(node->keys.begin(), node->keys.end(), key);
            if (it != node->keys.end() && *it == key) {
                int pos = it - node->keys.begin();
                return node->values[pos];
            }
            return -1;
        }

        int index = findChildIndex(node, key);
        node = node->children[index];
    }

    return -1;
}

int BPlusTree::findKey(std::shared_ptr<BPlusTreeNode> node, int64_t key) {
    auto it = std::lower_bound(node->keys.begin(), node->keys.end(), key);
    return it - node->keys.begin();
}

int BPlusTree::findChildIndex(std::shared_ptr<BPlusTreeNode> node, int64_t key) {
    int index = 0;
    while (index < (int)node->keys.size() && key >= node->keys[index]) {
        index++;
    }
    return index;
}

std::vector<int> BPlusTree::rangeSearch(int64_t min, int64_t max) {
    std::vector<int> results;

    // 找到最小键的叶子节点
    auto node = root_;
    while (node && !node->isLeaf) {
        int index = 0;
        while (index < (int)node->keys.size() && min >= node->keys[index]) {
            index++;
        }
        node = node->children[index];
    }

    // 在叶子节点链表中查找
    while (node) {
        for (size_t i = 0; i < node->keys.size(); i++) {
            if (node->keys[i] > max) break;
            if (node->keys[i] >= min) {
                results.push_back(node->values[i]);
            }
        }
        node = node->next;
    }

    return results;
}

bool BPlusTree::remove(int64_t key) {
    if (find(key) == -1) {
        return false;
    }

    root_ = remove(root_, key);

    // 如果根节点只有一个孩子，降低树高
    if (!root_->isLeaf && root_->children.size() == 1) {
        root_ = root_->children[0];
    }

    return true;
}

std::shared_ptr<BPlusTreeNode> BPlusTree::remove(std::shared_ptr<BPlusTreeNode> node, int64_t key) {
    // 简化实现：仅标记删除，实际需要完整的 B+树删除逻辑
    // 完整实现包括：从叶子删除、借键、合并节点等
    // 这里提供基本框架

    if (node->isLeaf) {
        int pos = findKey(node, key);
        if (pos < (int)node->keys.size() && node->keys[pos] == key) {
            node->keys.erase(node->keys.begin() + pos);
            node->values.erase(node->values.begin() + pos);
        }
        return node;
    }

    int index = findChildIndex(node, key);
    node->children[index] = remove(node->children[index], key);

    return node;
}

void BPlusTree::clear() {
    root_ = std::make_shared<BPlusTreeNode>(true);
}
