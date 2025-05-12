#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <stack>
#include <utility>
#include <vector>
static std::mt19937
    rng(std::chrono::steady_clock::now().time_since_epoch().count());

typedef long long ll;
class NodePool {
private:
  static const size_t BLOCK_SIZE = 1 << 20;
  std::vector<char *> blocks;
  char *current_block;
  size_t current_pos;

public:
  NodePool() : current_block(nullptr), current_pos(0) {}
  ~NodePool() {
    for (char *block : blocks) {
      delete[] block;
    }
  }

  void *allocate(size_t size) {
    if (!current_block || current_pos + size > BLOCK_SIZE) {
      current_block = new char[BLOCK_SIZE];
      blocks.push_back(current_block);
      current_pos = 0;
    }
    void *ptr = current_block + current_pos;
    current_pos += size;
    return ptr;
  }
};

NodePool global_node_pool;

class ESet {
private:
  // Treap 节点定义，带有引用计数以支持持久化
  struct Node {
    ll key;             // 关键字
    int priority;       // Treap 随机优先级
    Node *left, *right; // 左右子节点
    int ref_count;      // 引用计数，用于持久化
    size_t size_;       // 当前子树的大小

    Node(ll k)
        : key(k), priority(rng()), left(nullptr), right(nullptr), ref_count(1),
          size_(1) {}
    Node(ll key, int priority, Node *left, Node *right)
        : key(key), priority(priority), left(left), right(right), ref_count(1) {
      size_ = 1;
      if (left)
        ++left->ref_count, size_ += left->size_;
      if (right)
        ++right->ref_count, size_ += right->size_;
    }
    static void *operator new(size_t size) {
      return global_node_pool.allocate(size);
    }

    static void operator delete(void *) {
      // 内存池不实际释放内存
    }
  };

  Node *root;
  size_t tree_size;

  // 合并两个 Treap，返回合并后的根节点
  // 保持 Treap 的平衡和二叉搜索树性质
  Node *merge(Node *left, Node *right) {
    if (!left) {
      return right;
    }
    if (!right) {
      return left;
    }

    if (left->priority > right->priority) {
      left->right = merge(left->right, right);
      left->size_ = 1 + (left->left ? left->left->size_ : 0) +
                    (left->right ? left->right->size_ : 0);
      return left;
    } else {
      right->left = merge(left, right->left);
      right->size_ = 1 + (right->left ? right->left->size_ : 0) +
                     (right->right ? right->right->size_ : 0);
      return right;
    }
  }

  // split_lower: 将所有严格小于 key 的节点分到左子树，其他分到右子树
  // 用于插入、删除等操作时分割 Treap
  void split_lower(Node *node, ll key, Node *&left_, Node *&right_) {
    if (!node) {
      left_ = right_ = nullptr;
      return;
    }
    if (node->key < key) {
      if (node->ref_count == 0) {
        left_ = node;
        ++node->ref_count;
        if (node->right)
          --node->right->ref_count;
      } else {
        left_ = new Node(node->key, node->priority, node->left, nullptr);
      }
      split_lower(node->right, key, left_->right, right_);
      left_->size_ = 1 + (left_->left ? left_->left->size_ : 0) +
                     (left_->right ? left_->right->size_ : 0);
    } else {
      if (node->ref_count == 0) {
        right_ = node;
        ++node->ref_count;
        if (node->left)
          --node->left->ref_count;
      } else {
        right_ = new Node(node->key, node->priority, nullptr, node->right);
      }
      split_lower(node->left, key, left_, right_->left);
      right_->size_ = 1 + (right_->left ? right_->left->size_ : 0) +
                      (right_->right ? right_->right->size_ : 0);
    }
  }

  // split_greater: 将所有大于等于 key 的节点分到右子树，其他分到左子树
  // 用于范围查询的右边界分割
  void split_greater(Node *node, ll key, Node *&left_, Node *&right_) {
    if (!node) {
      left_ = right_ = nullptr;
      return;
    }
    if (key >= node->key) {
      if (node->ref_count == 0) {
        left_ = node;
        ++node->ref_count;
        if (node->right)
          --node->right->ref_count;
      } else {
        left_ = new Node(node->key, node->priority, node->left, nullptr);
      }
      split_greater(node->right, key, left_->right, right_);
      left_->size_ = 1 + (left_->left ? left_->left->size_ : 0) +
                     (left_->right ? left_->right->size_ : 0);
    } else {
      if (node->ref_count == 0) {
        right_ = node;
        ++node->ref_count;
        if (node->left)
          --node->left->ref_count;
      } else {
        right_ = new Node(node->key, node->priority, nullptr, node->right);
      }
      split_greater(node->left, key, left_, right_->left);
      right_->size_ = 1 + (right_->left ? right_->left->size_ : 0) +
                      (right_->right ? right_->right->size_ : 0);
    }
  }

  // 递归释放节点，引用计数为0时删除节点
  void clear(Node *node) {
    if (node && --node->ref_count == 0) {
      clear(node->left);
      clear(node->right);
      delete node;
      node = nullptr;
    }
  }

  // 更新当前集合的最小值和最大值

  void updateMin() {
    if (empty())
      return;
    Node *curr = root;
    while (curr->left)
      curr = curr->left;
    minK = curr->key;
  }

  void updateMax() {
    if (empty())
      return;
    Node *curr = root;
    while (curr->right)
      curr = curr->right;
    maxK = curr->key;
  }

public:
  ll minK, maxK;

  ESet() : root(nullptr), tree_size(0), minK(0), maxK(0) { srand(time(0)); }

  ESet(const ESet &other)
      : root(other.root), tree_size(other.tree_size), minK(other.minK),
        maxK(other.maxK) {
    if (root)
      ++root->ref_count;
  }

  ESet &operator=(const ESet &other) {
    if (this != &other) {
      clear(root);
      root = other.root;
      tree_size = other.tree_size;
      minK = other.minK;
      maxK = other.maxK;
      if (root)
        root->ref_count++;
    }
    return *this;
  }

  ~ESet() {
    clear(root);
    root = nullptr;
  }

  // 插入元素，若元素已存在返回 false，否则插入并返回 true
  bool emplace(ll key) {
    if (contains(key))
      return false;
    if (empty()) {
      root = new Node(key);
      minK = maxK = key;
      tree_size = 1;
      return true;
    }
    if (key < minK)
      minK = key;
    if (key > maxK)
      maxK = key;
    Node *left = nullptr, *right = nullptr;
    --root->ref_count;

    split_lower(root, key, left, right);
    Node *new_node = new Node(key);
    root = merge(merge(left, new_node), right);
    tree_size++;

    return true;
  }

  // 删除元素，返回删除成功与否（0或1）
  size_t erase(ll key) {
    if (!contains(key))
      return 0;
    Node *left = nullptr, *mid1 = nullptr, *mid2 = nullptr, *right = nullptr;
    --root->ref_count;

    split_lower(root, key, left, mid1);
    if (mid1)
      --mid1->ref_count;

    split_greater(mid1, key, mid2, right);
    root = merge(left, right);
    if (mid2) {
      tree_size--;
      clear(mid2);
      if (key == minK)
        updateMin();
      if (key == maxK)
        updateMax();
      if (!tree_size)
        root = nullptr;
      return 1;
    } else {
      return 0;
    }
  }

  // 判断元素是否存在
  inline bool contains(ll key) const {
    if (empty())
      return false;
    Node *node = root;
    while (node) {
      if (key < node->key)
        node = node->left;
      else if (node->key < key)
        node = node->right;
      else
        return true;
    }
    return false;
  }

  // 统计区间 [l, r] 内元素数量
  size_t range(ll l, ll r) {
    if (empty())
      return 0;
    Node *left = nullptr, *mid1 = nullptr, *mid2 = nullptr, *right = nullptr;
    --root->ref_count;

    split_lower(root, l, left, mid1);
    if (mid1)
      --mid1->ref_count;
    split_greater(mid1, r, mid2, right);
    size_t res = mid2 ? mid2->size_ : 0;
    root = merge(merge(left, mid2), right);
    return res;
  }

  // 查找小于 key 的最大元素（前驱），不存在返回 -1
  inline ll predecessor(ll key) const {
    Node *node = root;
    ll pred = -1;
    while (node) {
      if (node->key < key) {
        pred = node->key;
        node = node->right;
      } else {
        node = node->left;
      }
    }
    return pred;
  }

  // 查找大于 key 的最小元素（后继），不存在返回 -1
  inline ll successor(ll key) const {
    Node *node = root;
    ll succ = -1;
    while (node) {
      if (key < node->key) {
        succ = node->key;
        node = node->left;
      } else {
        node = node->right;
      }
    }
    return succ;
  }

  inline size_t size() const { return tree_size; }
  inline bool empty() const { return tree_size == 0; }
};
// This program implements a persistent set using treap with copy-on-write
// semantics. Supported operations (via standard input):
// 0 a b — emplace value b into set s[a]
// 1 a b — erase value b from set s[a]
// 2 a   — copy set s[a] into a new set s.back()
// 3 a b — check if value b exists in set s[a]
// 4 a b c — count elements in set s[a] within range [b, c]
// 5     — if valid iterator, move it backward and print value, else print -1
// 6     — if valid iterator, move it forward and print value, else print -1
int main() {

  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  std::vector<ESet> sets(1);
  int op, lst = 0, it_a = -1;
  ll it_b = -1;
  bool valid = false;

  while (std::cin >> op) {
    ll a, b, c;
    switch (op) {
    case 0:
      // 插入元素 b 到集合 s[a]
      std::cin >> a >> b;
      if (a >= sets.size())
        sets.resize(a + 1);
      if (sets[a].emplace(b)) {
        it_a = a;
        it_b = b;
        valid = true;
      }
      break;
    case 1:
      // 从集合 s[a] 删除元素 b
      std::cin >> a >> b;
      if (valid && it_a == a && it_b == b)
        valid = false;
      sets[a].erase(b);
      break;
    case 2:
      // 复制集合 s[a] 到新的集合 s.back()
      std::cin >> a;
      sets.push_back(sets[a]);
      break;
    case 3:
      // 查询集合 s[a] 是否包含元素 b
      std::cin >> a >> b;
      if (a < sets.size() && sets[a].contains(b)) {
        std::cout << "true\n";
        it_a = a;
        it_b = b;
        valid = true;
      } else {
        std::cout << "false\n";
      }
      break;
    case 4:
      // 查询集合 s[a] 中区间 [b, c] 内的元素数量
      std::cin >> a >> b >> c;
      std::cout << sets[a].range(b, c) << '\n';

      break;
    case 5:
      // 如果迭代器有效，向前移动并输出当前元素，否则输出 -1
      if (valid) {
        ll pred = sets[it_a].predecessor(it_b);
        if (pred != -1 && pred < it_b) {
          it_b = pred;
          std::cout << it_b << '\n';
        } else {
          valid = false;
          std::cout << "-1\n";
        }
      } else {
        std::cout << "-1\n";
      }
      break;
    case 6:
      // 如果迭代器有效，向后移动并输出当前元素，否则输出 -1
      if (valid) {
        ll succ = sets[it_a].successor(it_b);
        if (succ != -1 && succ > it_b) {
          it_b = succ;
          std::cout << it_b << '\n';
        } else {
          valid = false;
          std::cout << "-1\n";
        }
      } else {
        std::cout << "-1\n";
      }
      break;
    }
  }
  return 0;
}