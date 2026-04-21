#include "BPlusTree.h"
#include "external/json.hpp"
#include "../common/Logger.h"
#include <algorithm>
#include <sstream>

using namespace minisql;
using namespace storage;
using json = nlohmann::json;

// ============================================================
// B+树构造函数
// ============================================================
// 参数 order: B+树的阶数m（每个节点最多m个孩子，m-1个键）
// 时间复杂度: O(1)
// ============================================================
BPlusTree::BPlusTree(int order) : order_(order) {
    minKeys_ = (order + 1) / 2;  // ceil(m/2)，节点最少键数
    root_ = std::make_shared<BPlusTreeNode>(true);  // 初始为空叶子节点
}

// ============================================================
// 插入操作（带并发锁）
// ============================================================
// Linux环境下使用读写锁保证并发安全
// Windows测试环境直接调用无锁版本
// ============================================================
bool BPlusTree::insert(int64_t key, int rowId) {
#ifdef __linux__
    RWLock::WriteGuard guard(rwLock_);  // 写锁保护整个插入过程
#endif
    return insertUnlocked(key, rowId);
}

// ============================================================
// B+树插入算法（核心逻辑）
// ============================================================
// 算法原理：
// 1. 检查键是否已存在，避免重复插入
// 2. 从根节点递归向下查找插入位置
// 3. 在叶子节点插入键值对
// 4. 如果叶子节点已满（键数 > order），执行分裂：
//    - 将节点分裂为左右两部分
//    - 分裂键上移到父节点
// 5. 如果父节点也满，继续向上分裂直到根节点
// 6. 根节点分裂时创建新根，树高度增加1
//
// 时间复杂度: O(log n) - 从根到叶子的路径长度
// 空间复杂度: O(log n) - 递归栈深度
// ============================================================
bool BPlusTree::insertUnlocked(int64_t key, int rowId) {
    // 检查键是否已存在，避免重复插入
    if (findUnlocked(key) != -1) {
        return false;
    }

    // 执行插入，splitKey 用于传递分裂产生的新键
    int64_t splitKey = 0;
    auto newRoot = insert(root_, key, rowId, splitKey);

    // 根节点分裂，创建新根节点，树高度+1
    if (newRoot) {
        auto oldRoot = root_;
        root_ = std::make_shared<BPlusTreeNode>(false);  // 新根为内部节点
        root_->keys.push_back(splitKey);        // 分裂键作为新根的唯一键
        root_->children.push_back(oldRoot);      // 左孩子：原根节点
        root_->children.push_back(newRoot);      // 右孩子：分裂产生的新节点
    }

    return true;
}

// ============================================================
// B+树递归插入函数（内部实现）
// ============================================================
// 参数：
//   node     - 当前节点
//   key      - 要插入的键
//   rowId    - 要插入的行ID（叶子节点的值）
//   splitKey - 输出参数，分裂时传递给父节点的键
//
// 返回值：
//   nullptr       - 插入成功且无需分裂
//   新节点指针    - 当前节点分裂，返回新创建的右节点
//
// 算法步骤：
//   1. 叶子节点：直接插入，检查是否需要分裂
//   2. 内部节点：递归插入到子节点，处理子节点分裂
// ============================================================
std::shared_ptr<BPlusTreeNode> BPlusTree::insert(std::shared_ptr<BPlusTreeNode> node, int64_t key, int rowId, int64_t& splitKey) {
    if (node->isLeaf) {
        // ===== 叶子节点处理 =====
        // 使用二分查找找到插入位置，保持有序
        auto it = std::lower_bound(node->keys.begin(), node->keys.end(), key);
        int pos = it - node->keys.begin();

        // 在找到的位置插入键和值
        node->keys.insert(it, key);
        node->values.insert(node->values.begin() + pos, rowId);

        // 检查是否需要分裂（键数超过阶数）
        if (node->keys.size() > (size_t)order_) {
            auto newNode = std::make_shared<BPlusTreeNode>(true);  // 新叶子节点
            size_t sz = node->keys.size();
            size_t mid = sz / 2;  // 分裂点位置

            // ===== 叶子节点分裂策略 =====
            // 分裂点键上移到父节点
            // 左叶子保留 [0, mid)，不包含分裂键
            // 右叶子保留 [mid, sz)，包含分裂键
            // 这样设计是为了保证叶子节点的连续性查询
            splitKey = node->keys[mid];

            // 将[mid, sz)复制到新节点（右叶子包含分裂键）
            for (size_t i = mid; i < sz; i++) {
                newNode->keys.push_back(node->keys[i]);
                newNode->values.push_back(node->values[i]);
            }

            // 左叶子截断到[0, mid)
            node->keys.resize(mid);
            node->values.resize(mid);

            // 维护叶子节点的链表指针（用于范围查询）
            newNode->next = node->next;
            node->next = newNode;

            return newNode;  // 返回分裂产生的右节点
        }
        return nullptr;  // 无需分裂
    }

    // ===== 内部节点处理 =====
    // 找到应该插入的孩子节点索引
    int index = findChildIndex(node, key);
    // 递归插入到子节点
    auto newChild = insert(node->children[index], key, rowId, splitKey);

    if (newChild) {
        // 子节点分裂，需要在本节点插入分裂键和新孩子指针
        // 在index位置插入分裂键
        node->keys.insert(node->keys.begin() + index, splitKey);
        // 在index+1位置插入新孩子（分裂产生的右节点）
        node->children.insert(node->children.begin() + index + 1, newChild);

        // 检查内部节点是否需要分裂
        if (node->keys.size() > (size_t)order_) {
            auto newNode = std::make_shared<BPlusTreeNode>(false);  // 新内部节点
            size_t sz = node->keys.size();
            size_t mid = sz / 2;  // 分裂点位置

            // ===== 内部节点分裂策略 =====
            // 分裂点键上移到父节点
            splitKey = node->keys[mid];

            // 左节点保留[0, mid)，左节点有mid个键，mid+1个孩子
            node->keys.resize(mid);

            // 右节点复制[mid+1, sz)，分裂键本身不保留在右节点
            // 右节点有sz-mid-1个键，sz-mid个孩子
            for (size_t i = mid + 1; i < sz; i++) {
                newNode->keys.push_back(node->keys[i]);
            }

            // 移动孩子指针到新节点
            // 左节点保留[0, mid+1)的孩子（mid+1个）
            // 右节点保留[mid+1, childCount)的孩子
            size_t childCount = node->children.size();
            for (size_t i = mid + 1; i < childCount; i++) {
                newNode->children.push_back(node->children[i]);
            }
            node->children.resize(mid + 1);  // 左节点保留mid+1个孩子

            return newNode;  // 返回分裂产生的右节点
        }
    }

    return nullptr;  // 无需分裂
}

// ============================================================
// 查找操作（带并发锁）
// ============================================================
// 返回值：找到返回row_id，未找到返回-1
// ============================================================
int BPlusTree::find(int64_t key) {
#ifdef __linux__
    RWLock::ReadGuard guard(rwLock_);  // 读锁，允许多线程并发读
#endif
    return findUnlocked(key);
}

// ============================================================
// B+树查找算法（核心逻辑）
// ============================================================
// 算法原理：
// 1. 从根节点开始向下查找
// 2. 内部节点：根据键值范围确定孩子节点索引
// 3. 叶子节点：使用二分查找定位具体键
// 4. 找到返回对应的row_id，未找到返回-1
//
// 时间复杂度: O(log n) - 树的高度
// ============================================================
int BPlusTree::findUnlocked(int64_t key) {
    auto node = root_;

    while (node) {
        if (node->isLeaf) {
            // 叶子节点：使用二分查找定位键
            auto it = std::lower_bound(node->keys.begin(), node->keys.end(), key);
            if (it != node->keys.end() && *it == key) {
                // 找到键，返回对应的row_id
                int pos = it - node->keys.begin();
                return node->values[pos];
            }
            return -1;  // 未找到
        }

        // 内部节点：确定应该查找的孩子分支
        int index = findChildIndex(node, key);
        node = node->children[index];
    }

    return -1;  // 树为空或未找到
}

// ============================================================
// 在节点中查找键的位置（辅助函数）
// ============================================================
// 使用二分查找返回键应该插入的位置索引
// 时间复杂度: O(log m)，m为节点键数
// ============================================================
int BPlusTree::findKey(std::shared_ptr<BPlusTreeNode> node, int64_t key) {
    auto it = std::lower_bound(node->keys.begin(), node->keys.end(), key);
    return it - node->keys.begin();
}

// ============================================================
// 在内部节点中查找孩子索引（辅助函数）
// ============================================================
// 根据键值范围确定应该访问的孩子分支
// 规则：
//   - keys[i] <= key < keys[i+1]，访问children[i+1]
//   - key < keys[0]，访问children[0]
//   - key >= keys.back()，访问children.back()
//
// 时间复杂度: O(m)，m为节点键数
// ============================================================
int BPlusTree::findChildIndex(std::shared_ptr<BPlusTreeNode> node, int64_t key) {
    int index = 0;
    // 线性扫描找到合适的分支（键数较少时比二分查找更快）
    while (index < (int)node->keys.size() && key >= node->keys[index]) {
        index++;
    }
    return index;
}

// ============================================================
// 范围查询操作（带并发锁）
// ============================================================
// 返回值：所有键在[min, max]范围内的row_id列表
// ============================================================
std::vector<int> BPlusTree::rangeSearch(int64_t min, int64_t max) {
#ifdef __linux__
    RWLock::ReadGuard guard(rwLock_);
#endif
    return rangeSearchUnlocked(min, max);
}

// ============================================================
// B+树范围查询算法（核心逻辑）
// ============================================================
// 算法原理：
// 1. 从根节点向下查找包含min键的叶子节点
// 2. 利用叶子节点的链表指针，顺序扫描所有键
// 3. 收集所有在[min, max]范围内的row_id
// 4. 遇到第一个大于max的键时停止
//
// 时间复杂度: O(log n + k)，n为树节点数，k为结果集大小
// ============================================================
std::vector<int> BPlusTree::rangeSearchUnlocked(int64_t min, int64_t max) {
    std::vector<int> results;

    // ===== 找到包含最小键的叶子节点 =====
    auto node = root_;
    while (node && !node->isLeaf) {
        // 在内部节点中确定应该访问的孩子分支
        int index = 0;
        while (index < (int)node->keys.size() && min >= node->keys[index]) {
            index++;
        }
        node = node->children[index];
    }

    // ===== 在叶子节点链表中顺序扫描 =====
    while (node) {
        for (size_t i = 0; i < node->keys.size(); i++) {
            // 超过范围上限，停止扫描
            if (node->keys[i] > max) break;
            // 键在范围内，收集对应的row_id
            if (node->keys[i] >= min) {
                results.push_back(node->values[i]);
            }
        }
        // 利用链表指针跳到下一个叶子节点
        node = node->next;
    }

    return results;
}

// ============================================================
// 删除操作（带并发锁）
// ============================================================
bool BPlusTree::remove(int64_t key) {
#ifdef __linux__
    RWLock::WriteGuard guard(rwLock_);  // 写锁保护删除过程
#endif
    return removeUnlocked(key);
}

// ============================================================
// B+树删除算法（简化实现）
// ============================================================
// 当前实现为简化版本：
// - 仅删除叶子节点的键值，未实现完整的B+树删除逻辑
// - 未处理节点合并、键借用等复杂情况
// - 未维护节点的最小键数约束
//
// 完整B+树删除算法应包括：
// 1. 从叶子节点删除键值
// 2. 如果节点键数 < minKeys_，执行：
//    a) 从左兄弟节点借用键（如果左兄弟有足够键）
//    b) 从右兄弟节点借用键（如果右兄弟有足够键）
//    c) 与兄弟节点合并（如果兄弟节点也无法借用）
// 3. 如果内部节点的孩子被合并，更新内部节点的键和孩子指针
// 4. 如果根节点只剩一个孩子，降低树高度
//
// 时间复杂度: O(log n) - 从根到叶子的路径长度
// ============================================================
bool BPlusTree::removeUnlocked(int64_t key) {
    // 检查键是否存在
    if (findUnlocked(key) == -1) {
        return false;
    }

    // 执行删除
    root_ = remove(root_, key);

    // 如果根节点只有一个孩子（内部节点），降低树高度
    if (!root_->isLeaf && root_->children.size() == 1) {
        root_ = root_->children[0];  // 唯一的孩子成为新根
    }

    return true;
}

std::shared_ptr<BPlusTreeNode> BPlusTree::remove(std::shared_ptr<BPlusTreeNode> node, int64_t key) {
    // 简化实现：仅标记删除，实际需要完整的 B+树删除逻辑
    // 完整实现包括：从叶子删除、借键、合并节点等
    // 这里提供基本框架

    if (node->isLeaf) {
        // 叶子节点：直接删除键和值
        int pos = findKey(node, key);
        if (pos < (int)node->keys.size() && node->keys[pos] == key) {
            node->keys.erase(node->keys.begin() + pos);
            node->values.erase(node->values.begin() + pos);
        }
        return node;
    }

    // 内部节点：递归删除
    int index = findChildIndex(node, key);
    node->children[index] = remove(node->children[index], key);

    return node;
}

// ============================================================
// 清空B+树
// ============================================================
void BPlusTree::clear() {
    root_ = std::make_shared<BPlusTreeNode>(true);  // 重置为空叶子节点
}

// ============================================================
// B+树序列化（持久化到JSON）
// ============================================================
// 序列化策略：
// 1. 使用前序遍历遍历所有节点
// 2. 每个节点记录：isLeaf、keys、values(叶子)、children数量(内部节点)
// 3. 记录父节点索引和孩子索引，用于反序列化重建结构
// 4. 叶子节点额外记录selfIndex，用于重建链表指针
//
// JSON格式示例：
// {
//   "order": 200,           // B+树阶数
//   "version": 1,           // 版本号
//   "rootIndex": 0,         // 根节点索引
//   "nodes": [              // 节点列表（前序遍历）
//     {"isLeaf": false, "keys": [10,20], "childCount": 3, ...},
//     {"isLeaf": true, "keys": [5,10], "values": [1,2], ...},
//     ...
//   ]
// }
// ============================================================
std::string BPlusTree::serialize() const {
    json j;
    j["order"] = order_;
    j["version"] = 1;

    // 使用扁平化的节点列表，前序遍历
    std::vector<std::string> nodes;
    serializeNode(root_, nodes, -1, 0);  // 从根节点开始，父索引-1表示根
    j["nodes"] = nodes;
    j["rootIndex"] = 0;  // 根节点总是第一个节点（索引0）

    return j.dump(2);  // 格式化输出，缩进为2
}

// ============================================================
// 序列化辅助函数（递归序列化节点）
// ============================================================
// 参数：
//   node        - 当前要序列化的节点
//   nodes       - 输出参数，序列化后的节点字符串列表
//   parentIndex - 父节点在nodes列表中的索引（根节点为-1）
//   childIndex  - 当前节点是父节点的第几个孩子（根节点为0）
// ============================================================
void BPlusTree::serializeNode(const std::shared_ptr<BPlusTreeNode>& node,
                              std::vector<std::string>& nodes,
                              int parentIndex,
                              int childIndex) const {
    json nodeJson;
    nodeJson["isLeaf"] = node->isLeaf;
    nodeJson["parentIndex"] = parentIndex;  // 用于反序列化时验证父子关系
    nodeJson["childIndex"] = childIndex;    // 用于确定孩子在父节点中的位置

    // 序列化键
    for (int64_t key : node->keys) {
        nodeJson["keys"].push_back(key);
    }

    // 序列化值（叶子节点）或子节点数量（内部节点）
    if (node->isLeaf) {
        // 叶子节点：序列化row_id值
        for (int val : node->values) {
            nodeJson["values"].push_back(val);
        }
        // 记录当前叶子节点在节点数组中的索引，用于恢复next指针
        nodeJson["selfIndex"] = nodes.size();
    } else {
        // 内部节点：记录孩子数量，用于反序列化时重建children指针
        nodeJson["childCount"] = node->children.size();
    }

    int currentIndex = nodes.size();  // 当前节点的索引
    nodes.push_back(nodeJson.dump());  // 将节点JSON字符串加入列表

    // 前序遍历子节点（非叶子节点）
    if (!node->isLeaf) {
        for (size_t i = 0; i < node->children.size(); ++i) {
            serializeNode(node->children[i], nodes, currentIndex, i);
        }
    }
}

// ============================================================
// B+树反序列化（从JSON恢复）
// ============================================================
// 反序列化步骤：
// 1. 第一遍：反序列化所有节点，恢复keys和values
// 2. 第二遍：恢复内部节点的children指针（根据childCount和前序遍历顺序）
// 3. 第三遍：恢复叶子节点的next指针（形成叶子链表）
// 4. 设置根节点
//
// 注意事项：
// - 使用前序遍历顺序，子节点紧跟父节点之后
// - 如果JSON解析失败，回退到空树
// - 反序列化完成后验证日志输出节点总数
// ============================================================
void BPlusTree::deserialize(const std::string& data) {
    try {
        json j = json::parse(data);

        // 恢复B+树参数
        order_ = j.value("order", 200);
        minKeys_ = (order_ + 1) / 2;

        int rootIndex = j.value("rootIndex", 0);
        std::vector<std::string> nodesJson = j["nodes"].get<std::vector<std::string>>();

        // ===== 第一遍：反序列化所有节点，恢复基本结构 =====
        std::vector<std::shared_ptr<BPlusTreeNode>> allNodes;
        allNodes.reserve(nodesJson.size());

        for (size_t i = 0; i < nodesJson.size(); ++i) {
            json nodeData = json::parse(nodesJson[i]);
            auto node = std::make_shared<BPlusTreeNode>(nodeData["isLeaf"]);

            // 反序列化键
            if (nodeData.contains("keys")) {
                for (auto& key : nodeData["keys"]) {
                    node->keys.push_back(key.get<int64_t>());
                }
            }

            // 反序列化值（仅叶子节点）
            if (nodeData.contains("values")) {
                for (auto& val : nodeData["values"]) {
                    node->values.push_back(val.get<int>());
                }
            }

            allNodes.push_back(node);
        }

        // ===== 第二遍：恢复内部节点的children指针 =====
        // 前序遍历特点：子节点紧跟在父节点之后
        for (size_t i = 0; i < nodesJson.size(); ++i) {
            json nodeData = json::parse(nodesJson[i]);
            if (!nodeData["isLeaf"]) {  // 内部节点
                int childCount = nodeData.value("childCount", 0);
                // 计算子节点的起始索引：父节点后的第一个节点就是第一个孩子
                size_t startIdx = i + 1;
                for (int c = 0; c < childCount; ++c) {
                    if (startIdx + c < allNodes.size()) {
                        allNodes[i]->children.push_back(allNodes[startIdx + c]);
                    }
                }
            }
        }

        // ===== 第三遍：恢复叶子节点的next指针（叶子链表） =====
        std::vector<std::shared_ptr<BPlusTreeNode>> leaves;
        for (auto& node : allNodes) {
            if (node->isLeaf) {
                leaves.push_back(node);
            }
        }
        // 按前序遍历顺序连接叶子节点
        for (size_t i = 0; i < leaves.size() - 1; ++i) {
            leaves[i]->next = leaves[i + 1];
        }
        if (!leaves.empty()) {
            leaves.back()->next = nullptr;  // 最后一个叶子节点的next为空
        }

        // ===== 设置根节点 =====
        if (rootIndex >= 0 && rootIndex < (int)allNodes.size()) {
            root_ = allNodes[rootIndex];
        } else {
            root_ = std::make_shared<BPlusTreeNode>(true);  // 回退到空树
        }

        LOG_INFO("BPlusTree", "Deserialized B+ tree with " +
                 std::to_string(allNodes.size()) + " nodes");

    } catch (const json::parse_error& e) {
        LOG_ERROR("BPlusTree", "Failed to parse index data: " + std::string(e.what()));
        // 回退到空树
        root_ = std::make_shared<BPlusTreeNode>(true);
    }
}

int BPlusTree::getNodeCount() const {
    int count = 0;
    std::function<void(const std::shared_ptr<BPlusTreeNode>&)> traverse =
        [&](const std::shared_ptr<BPlusTreeNode>& node) {
            if (!node) return;
            count++;
            if (!node->isLeaf) {
                for (const auto& child : node->children) {
                    traverse(child);
                }
            }
        };
    traverse(root_);
    return count;
}
