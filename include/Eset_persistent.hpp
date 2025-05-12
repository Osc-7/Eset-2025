#ifndef SJTU_ESET_HPP
#define SJTU_ESET_HPP

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

template <typename T> struct DefaultLess {
  bool operator()(const T &a, const T &b) const { return a < b; }
};

template <class Key, class Compare = DefaultLess<Key>> class ESet {
private:
  enum Color { RED, BLACK };

  // Node structure for persistent red-black tree
  struct Node {
    Key key;
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;
    std::weak_ptr<Node> parent; // 添加父指针
    Color color;
    size_t ref_count;

    Node(const Key &k, std::shared_ptr<Node> l = nullptr,
         std::shared_ptr<Node> r = nullptr, Color c = RED)
        : key(k), left(l), right(r), color(c), ref_count(1) {}
  };

  using NodePtr = std::shared_ptr<Node>;

  // Persistent Red-Black Tree implementation
  class RBTree {
  private:
    NodePtr root;
    size_t node_count;

  public:
    Compare comp;

  private:
    // Helper function to create a new node with shared_ptr
    static NodePtr make_node(const Key &key, NodePtr left = nullptr,
                             NodePtr right = nullptr, Color color = RED) {
      NodePtr node = std::make_shared<Node>(key, left, right, color);
      if (left)
        left->parent = node;
      if (right)
        right->parent = node;
      return node;
    }

    // Persistent insert implementation
    NodePtr insert(NodePtr x, const Key &key, bool &inserted) const {
      if (!x) {
        inserted = true;
        return make_node(key, nullptr, nullptr, RED);
      }

      if (comp(key, x->key)) {
        NodePtr new_left = insert(x->left, key, inserted);
        if (inserted) {
          // Only create new nodes if insertion actually happened
          return make_node(x->key, new_left, x->right, x->color);
        }
        return x;
      } else if (comp(x->key, key)) {
        NodePtr new_right = insert(x->right, key, inserted);
        if (inserted) {
          return make_node(x->key, x->left, new_right, x->color);
        }
        return x;
      } else {
        inserted = false;
        return x;
      }
    }

    // Persistent erase implementation
    NodePtr erase(NodePtr x, const Key &key, bool &erased) const {
      if (!x) {
        erased = false;
        return x;
      }

      if (comp(key, x->key)) {
        NodePtr new_left = erase(x->left, key, erased);
        if (erased) {
          return make_node(x->key, new_left, x->right, x->color);
        }
        return x;
      } else if (comp(x->key, key)) {
        NodePtr new_right = erase(x->right, key, erased);
        if (erased) {
          return make_node(x->key, x->left, new_right, x->color);
        }
        return x;
      } else {
        erased = true;
        if (!x->left) {
          return x->right;
        } else if (!x->right) {
          return x->left;
        } else {
          // Node has two children, find successor
          NodePtr successor = x->right;
          while (successor->left) {
            successor = successor->left;
          }
          // Remove successor (which is guaranteed to have at most one child)
          bool dummy;
          NodePtr new_right = erase(x->right, successor->key, dummy);
          return make_node(successor->key, x->left, new_right, x->color);
        }
      }
    }

  public:
    RBTree() : root(nullptr), node_count(0), comp(Compare()) {}
    RBTree(NodePtr r, size_t cnt, Compare c)
        : root(r), node_count(cnt), comp(c) {}

    // Copy is trivial due to shared_ptr
    RBTree(const RBTree &other) = default;
    RBTree &operator=(const RBTree &other) = default;

    // Move is trivial due to shared_ptr
    RBTree(RBTree &&other) noexcept = default;
    RBTree &operator=(RBTree &&other) noexcept = default;
    NodePtr predecessor(NodePtr x) const {
      if (x->left)
        return maximum(x->left);
      NodePtr y = x->parent.lock();
      while (y && x == y->left) {
        x = y;
        y = y->parent.lock();
      }
      return y;
    }

    NodePtr successor(NodePtr x) const {
      if (x->right)
        return minimum(x->right);
      NodePtr y = x->parent.lock();
      while (y && x == y->right) {
        x = y;
        y = y->parent.lock();
      }
      return y;
    }

    // Find minimum node in subtree
    NodePtr minimum(NodePtr x) const {
      while (x && x->left) {
        x = x->left;
      }
      return x;
    }

    // Find maximum node in subtree
    NodePtr maximum(NodePtr x) const {
      while (x && x->right) {
        x = x->right;
      }
      return x;
    }

    // Find node with given key
    NodePtr find(NodePtr x, const Key &key) const {
      while (x) {
        if (comp(key, x->key)) {
          x = x->left;
        } else if (comp(x->key, key)) {
          x = x->right;
        } else {
          return x;
        }
      }
      return nullptr;
    }

    // Lower bound implementation
    NodePtr lower_bound(NodePtr x, const Key &key) const {
      NodePtr result = nullptr;
      while (x) {
        if (!comp(x->key, key)) {
          result = x;
          x = x->left;
        } else {
          x = x->right;
        }
      }
      return result;
    }

    // Upper bound implementation
    NodePtr upper_bound(NodePtr x, const Key &key) const {
      NodePtr result = nullptr;
      while (x) {
        if (comp(key, x->key)) {
          result = x;
          x = x->left;
        } else {
          x = x->right;
        }
      }
      return result;
    }

    // Public insert interface
    std::pair<NodePtr, bool> insert(const Key &key) {
      bool inserted = false;
      NodePtr new_root = insert(root, key, inserted);
      if (inserted) {
        return {new_root, true};
      } else {
        return {root, false};
      }
    }

    // Public erase interface
    std::pair<NodePtr, bool> erase(const Key &key) {
      bool erased = false;
      NodePtr new_root = erase(root, key, erased);
      if (erased) {
        return {new_root, true};
      } else {
        return {root, false};
      }
    }

    // Update the tree with new root and count
    void update(NodePtr new_root, size_t delta) {
      root = new_root;
      node_count += delta;
    }

    // Getters
    NodePtr getRoot() const { return root; }
    size_t size() const { return node_count; }
  };

  RBTree tree;

public:
  // Simplified iterator implementation
  class const_iterator {
  private:
    NodePtr node;
    const RBTree *tree;

  public:
    const_iterator(NodePtr n = nullptr, const RBTree *t = nullptr)
        : node(n), tree(t) {}

    const Key &operator*() const {
      if (!node) {
        throw std::out_of_range("dereferencing end iterator");
      }
      return node->key;
    }
    // 前置递减
    const_iterator &operator--() {
      if (!node) {
        if (!tree || !tree->getRoot())
          return *this;
        node = tree->maximum(tree->getRoot());
      } else {
        node = tree->predecessor(node);
      }
      return *this;
    }

    // 后置递减
    const_iterator operator--(int) {
      const_iterator tmp = *this;
      --(*this);
      return tmp;
    }

    // 前置递增
    const_iterator &operator++() {
      if (!node)
        return *this;
      node = tree->successor(node);
      return *this;
    }

    // 后置递增
    const_iterator operator++(int) {
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    // 比较操作
    bool operator==(const const_iterator &rhs) const {
      return node == rhs.node;
    }

    bool operator!=(const const_iterator &rhs) const {
      return node != rhs.node;
    }
  };

  using iterator = const_iterator;

  ESet() = default;
  ~ESet() = default;

  // Copy and move operations are O(1) due to shared_ptr
  ESet(const ESet &other) = default;
  ESet &operator=(const ESet &other) = default;
  ESet(ESet &&other) noexcept = default;
  ESet &operator=(ESet &&other) noexcept = default;

  // Insert element
  std::pair<iterator, bool> emplace(const Key &key) {
    auto [new_root, inserted] = tree.insert(key);
    if (inserted) {
      tree.update(new_root, 1);
      return {iterator(new_root, &tree), true};
    }
    return {iterator(tree.find(tree.getRoot(), key), &tree), false};
  }

  // Erase element
  size_t erase(const Key &key) {
    auto [new_root, erased] = tree.erase(key);
    if (erased) {
      tree.update(new_root, -1);
      return 1;
    }
    return 0;
  }

  // Find element
  iterator find(const Key &key) const {
    return iterator(tree.find(tree.getRoot(), key), &tree);
  }

  // Lower bound
  iterator lower_bound(const Key &key) const {
    return iterator(tree.lower_bound(tree.getRoot(), key), &tree);
  }

  // Upper bound
  iterator upper_bound(const Key &key) const {
    return iterator(tree.upper_bound(tree.getRoot(), key), &tree);
  }

  // Range count
  size_t range(const Key &l, const Key &r) const {
    if (tree.comp(r, l))
      return 0;

    size_t cnt = 0;
    auto it = lower_bound(l);
    auto end = upper_bound(r);

    while (it != end) {
      ++cnt;
      ++it;
    }
    return cnt;
  }

  // Begin iterator
  iterator begin() const {
    return iterator(tree.minimum(tree.getRoot()), &tree);
  }

  // End iterator
  iterator end() const { return iterator(nullptr, &tree); }

  // Size
  size_t size() const { return tree.size(); }

  // Clear
  void clear() { tree = RBTree(); }
};

#endif