// #include "RB_tree.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <utility>

template <typename Key, typename Compare = std::less<Key>> class RBTree {
  template <typename, typename> class ESet; // 前向声明

  friend class ESet<Key, Compare>;

private:
  enum Color { RED, BLACK };

public:
  struct Node {
    Key key;
    Node *parent;
    Node *left;
    Node *right;
    Color color;

    Node(const Key &k)
        : key(k), parent(nullptr), left(nullptr), right(nullptr), color(RED) {}
  };

  Node *root;
  size_t node_count;
  Compare comp;

private:
  // 左旋
  void leftRotate(Node *x) {
    Node *y = x->right;
    x->right = y->left;
    if (y->left)
      y->left->parent = x;
    y->parent = x->parent;
    if (!x->parent)
      root = y;
    else if (x == x->parent->left)
      x->parent->left = y;
    else
      x->parent->right = y;
    y->left = x;
    x->parent = y;
  }

  // 右旋
  void rightRotate(Node *x) {
    Node *y = x->left;
    x->left = y->right;
    if (y->right)
      y->right->parent = x;
    y->parent = x->parent;
    if (!x->parent)
      root = y;
    else if (x == x->parent->right)
      x->parent->right = y;
    else
      x->parent->left = y;
    y->right = x;
    x->parent = y;
  }

  // 插入修正，保持红黑树性质
  void insertFix(Node *z) {
    while (z->parent && z->parent->color == RED) {
      if (z->parent == z->parent->parent->left) {
        Node *y = z->parent->parent->right;
        if (y && y->color == RED) {
          z->parent->color = BLACK;
          y->color = BLACK;
          z->parent->parent->color = RED;
          z = z->parent->parent;
        } else {
          if (z == z->parent->right) {
            z = z->parent;
            leftRotate(z);
          }
          z->parent->color = BLACK;
          z->parent->parent->color = RED;
          rightRotate(z->parent->parent);
        }
      } else {
        Node *y = z->parent->parent->left;
        if (y && y->color == RED) {
          z->parent->color = BLACK;
          y->color = BLACK;
          z->parent->parent->color = RED;
          z = z->parent->parent;
        } else {
          if (z == z->parent->left) {
            z = z->parent;
            rightRotate(z);
          }
          z->parent->color = BLACK;
          z->parent->parent->color = RED;
          leftRotate(z->parent->parent);
        }
      }
    }
    root->color = BLACK;
  }

  void eraseFix(Node *x, Node *x_parent) {
    while ((x != root) && (!x || x->color == BLACK)) {
      if (x == (x_parent ? x_parent->left : nullptr)) {
        Node *w = x_parent ? x_parent->right : nullptr;
        if (w && w->color == RED) {
          w->color = BLACK;
          x_parent->color = RED;
          leftRotate(x_parent);
          w = x_parent->right;
        }
        if ((!w || (!w->left || w->left->color == BLACK) &&
                       (!w->right || w->right->color == BLACK))) {
          if (w)
            w->color = RED;
          x = x_parent;
          x_parent = x ? x->parent : nullptr;
        } else {
          if (!w->right || w->right->color == BLACK) {
            if (w->left)
              w->left->color = BLACK;
            if (w)
              w->color = RED;
            rightRotate(w);
            w = x_parent ? x_parent->right : nullptr;
          }
          if (w)
            w->color = x_parent->color;
          if (x_parent)
            x_parent->color = BLACK;
          if (w && w->right)
            w->right->color = BLACK;
          leftRotate(x_parent);
          x = root;
          break;
        }
      } else {
        Node *w = x_parent ? x_parent->left : nullptr;
        if (w && w->color == RED) {
          w->color = BLACK;
          x_parent->color = RED;
          rightRotate(x_parent);
          w = x_parent->left;
        }
        if ((!w || (!w->left || w->left->color == BLACK) &&
                       (!w->right || w->right->color == BLACK))) {
          if (w)
            w->color = RED;
          x = x_parent;
          x_parent = x ? x->parent : nullptr;
        } else {
          if (!w->left || w->left->color == BLACK) {
            if (w->right)
              w->right->color = BLACK;
            if (w)
              w->color = RED;
            leftRotate(w);
            w = x_parent ? x_parent->left : nullptr;
          }
          if (w)
            w->color = x_parent->color;
          if (x_parent)
            x_parent->color = BLACK;
          if (w && w->left)
            w->left->color = BLACK;
          rightRotate(x_parent);
          x = root;
          break;
        }
      }
    }
    if (x)
      x->color = BLACK;
  }

public:
  RBTree() : root(nullptr), node_count(0), comp(Compare()) {}
  ~RBTree() { clear(root); }

  // 移动构造
  RBTree(RBTree &&other) noexcept
      : root(other.root), node_count(other.node_count),
        comp(std::move(other.comp)) {
    other.root = nullptr;
    other.node_count = 0;
  }

  // 移动赋值
  RBTree &operator=(RBTree &&other) noexcept {
    if (this != &other) {
      clear(root); // 释放当前树
      root = other.root;
      node_count = other.node_count;
      comp = std::move(other.comp);
      other.root = nullptr;
      other.node_count = 0;
    }
    return *this;
  }

  // 插入操作
  std::pair<Node *, bool> insert(const Key &key) {
    Node *y = nullptr;
    Node *x = root;
    while (x) {
      y = x;
      if (comp(key, x->key))
        x = x->left;
      else if (comp(x->key, key))
        x = x->right;
      else
        return {x, false}; // 元素已存在
    }
    Node *z = new Node(key);
    z->parent = y;
    if (!y)
      root = z;
    else if (comp(z->key, y->key))
      y->left = z;
    else
      y->right = z;
    insertFix(z);
    ++node_count;
    return {z, true};
  }

  bool erase(const Key &key) {
    Node *z = find(key);
    if (!z)
      return false;

    Node *y = z;
    Node *x = nullptr;
    Node *x_parent = nullptr;
    Color y_original_color = y->color;

    if (!z->left) {
      x = z->right;
      x_parent = z->parent;
      if (x)
        x->parent = z->parent;
      if (!z->parent)
        root = x;
      else if (z == z->parent->left)
        z->parent->left = x;
      else
        z->parent->right = x;
    } else if (!z->right) {
      x = z->left;
      x_parent = z->parent;
      if (x)
        x->parent = z->parent;
      if (!z->parent)
        root = x;
      else if (z == z->parent->left)
        z->parent->left = x;
      else
        z->parent->right = x;
    } else {
      y = minimum(z->right);
      y_original_color = y->color;
      x = y->right;
      if (y->parent == z) {
        if (x)
          x->parent = y;
        x_parent = y;
      } else {
        if (x)
          x->parent = y->parent;
        y->parent->left = x;
        y->right = z->right;
        if (y->right)
          y->right->parent = y;
        x_parent = y->parent;
      }

      if (!z->parent)
        root = y;
      else if (z == z->parent->left)
        z->parent->left = y;
      else
        z->parent->right = y;
      y->parent = z->parent;
      y->left = z->left;
      if (y->left)
        y->left->parent = y;
      y->color = z->color;
    }

    delete z;
    --node_count;

    if (y_original_color == BLACK)
      eraseFix(x, x_parent);

    return true;
  };

  Node *find(const Key &key) const {
    Node *x = root;
    while (x) {
      if (comp(key, x->key))
        x = x->left;
      else if (comp(x->key, key))
        x = x->right;
      else
        return x;
    }
    return nullptr;
  };

  Node *minimum(Node *x) const {
    while (x && x->left)
      x = x->left;
    return x;
  }

  Node *maximum(Node *x) const {
    while (x && x->right)
      x = x->right;
    return x;
  }

  Node *successor(Node *node) const {
    if (node->right)
      return minimum(node->right);
    Node *y = node->parent;
    while (y && node == y->right) {
      node = y;
      y = y->parent;
    }
    return y;
  }

  Node *predecessor(Node *node) const {
    if (node->left)
      return maximum(node->left);
    Node *y = node->parent;
    while (y && node == y->left) {
      node = y;
      y = y->parent;
    }
    return y;
  }

  Node *lower_bound(const Key &key) const {
    Node *x = root;
    Node *res = nullptr;
    while (x) {
      if (!comp(x->key, key)) {
        res = x;
        x = x->left;
      } else {
        x = x->right;
      }
    }
    return res;
  };

  Node *upper_bound(const Key &key) const {
    Node *x = root;
    Node *res = nullptr;
    while (x) {
      if (comp(key, x->key)) {
        res = x;
        x = x->left;
      } else {
        x = x->right;
      }
    }
    return res;
  };

  size_t size() const noexcept { return node_count; }
  bool empty() const noexcept { return node_count == 0; }

  // Clear all nodes recursively
  void clear(Node *node) {
    if (!node)
      return;
    clear(node->left);
    clear(node->right);
    delete node;
  }
};

template <typename Key, typename Compare = std::less<Key>> class ESet {

private:
  RBTree<Key, Compare> tree;

public:
  ESet() = default;
  ~ESet() { clear(); }
  ESet(const ESet &other) {
    clear();
    for (auto it = other.begin(); it != other.end(); ++it)
      emplace(*it);
  }
  ESet &operator=(const ESet &other) {
    if (this != &other) {
      clear();
      for (auto it = other.begin(); it != other.end(); ++it)
        emplace(*it);
    }
    return *this;
  }

  ESet(ESet &&other) noexcept {
    tree = std::move(other.tree);
    other.clear();
  }

  ESet &operator=(ESet &&other) noexcept {
    if (this != &other) {
      clear();
      tree = std::move(other.tree);
      other.clear();
    }
    return *this;
  }

  void clear() {
    tree.clear(tree.root);
    tree.root = nullptr;
    tree.node_count = 0;
  }

  class iterator {
  private:
    const RBTree<Key, Compare> *tree;
    typename RBTree<Key, Compare>::Node *node;

  public:
    iterator(const RBTree<Key, Compare> *t = nullptr,
             typename RBTree<Key, Compare>::Node *n = nullptr)
        : tree(t), node(n) {}

    const Key &operator*() const {
      if (!node)
        throw std::out_of_range("dereferencing end iterator");
      return node->key;
    }

    iterator &operator++() {
      if (node)
        node = tree->successor(node);
      return *this;
    }

    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    iterator &operator--() {
      if (!node) {
        node = tree->maximum(tree->root);
      } else {
        auto min = tree->minimum(tree->root);
        if (node != min)
          node = tree->predecessor(node);
        // else already at begin(), do nothing
      }
      return *this;
    }

    iterator operator--(int) {
      iterator tmp = *this;
      --(*this);
      return tmp;
    }

    bool operator==(const iterator &other) const { return node == other.node; }
    bool operator!=(const iterator &other) const { return node != other.node; }
  };

  class const_iterator {};
  using const_iterator = iterator;

  template <class... Args> std::pair<iterator, bool> emplace(Args &&...args) {
    Key key(std::forward<Args>(args)...);
    auto [node, inserted] = tree.insert(key);
    return {iterator(&tree, node), inserted};
  }

  size_t erase(const Key &key) { return tree.erase(key); };
  iterator find(const Key &key) const {
    return iterator(&tree, tree.find(key));
  }

  size_t range(const Key &l, const Key &r) const {
    if (Compare()(r, l))
      return 0;
    size_t cnt = 0;
    for (auto it = lower_bound(l);
         it != end() && Compare()(it.operator*(), r) == false; ++it)
      ++cnt;
    if (find(r) != end())
      ++cnt;
    return cnt;
  };
  size_t size() const noexcept { return tree.size(); };
  iterator lower_bound(const Key &key) const {
    return iterator(&tree, tree.lower_bound(key));
  }
  iterator upper_bound(const Key &key) const {
    return iterator(&tree, tree.upper_bound(key));
  }
  iterator begin() const noexcept {
    return iterator(&tree, tree.minimum(tree.root));
  }
  iterator end() const noexcept { return iterator(&tree, nullptr); }
};
