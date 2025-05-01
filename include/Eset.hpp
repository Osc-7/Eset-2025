#ifndef SJTU_ESET_HPP
#define SJTU_ESET_HPP

#include <algorithm>
#include <stdexcept>
#include <utility>
// Task 1
//  ESet: A balanced ordered set container implemented using a red-black tree.
//  Supports insertion, deletion, search, and range queries with logarithmic
//  complexity.

template <typename T> struct DefaultLess {
  bool operator()(const T &a, const T &b) const { return a < b; }
};

template <class Key, class Compare = DefaultLess<Key>> class ESet {
private:
  enum Color { RED, BLACK };

  // Node structure for red-black tree
  struct Node {
    Key key;
    Node *parent;
    Node *left;
    Node *right;
    Color color;

    Node(const Key &k, Node *p = nullptr, Node *l = nullptr, Node *r = nullptr,
         Color c = RED)
        : key(k), parent(p), left(l), right(r), color(c) {}
  };

  // Red-Black Tree implementation
  class RBTree {
    friend class ESet<Key, Compare>;

  private:
    Node *root;
    size_t node_count;
    Compare comp;

    // Left rotate around node x
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

    // Right rotate around node x
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

    // Fix red-black tree properties after insertion of node z
    void insertFixup(Node *z) {
      while (z->parent && z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
          Node *y = z->parent->parent->right;
          // Case 1: Uncle y is red, recolor and move up the tree
          if (y && y->color == RED) {
            z->parent->color = BLACK;
            y->color = BLACK;
            z->parent->parent->color = RED;
            z = z->parent->parent;
          } else {
            // Case 2: Uncle y is black and z is right child, rotate left
            if (z == z->parent->right) {
              z = z->parent;
              leftRotate(z);
            }
            // Case 3: Uncle y is black and z is left child, rotate right
            z->parent->color = BLACK;
            z->parent->parent->color = RED;
            rightRotate(z->parent->parent);
          }
        } else {
          Node *y = z->parent->parent->left;
          // Symmetric cases for right subtree
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

    // Fix red-black tree properties after deletion of a node
    void eraseFixup(Node *x, Node *x_parent) {
      while (x != root && (!x || x->color == BLACK)) {
        if (x == (x_parent ? x_parent->left : nullptr)) {
          Node *w = x_parent ? x_parent->right : nullptr;
          // Case 1: Sibling w is red
          if (w && w->color == RED) {
            w->color = BLACK;
            x_parent->color = RED;
            leftRotate(x_parent);
            w = x_parent->right;
          }
          // Case 2: Sibling w's children are black
          if ((!w || (!w->left || w->left->color == BLACK) &&
                         (!w->right || w->right->color == BLACK))) {
            if (w)
              w->color = RED;
            x = x_parent;
            x_parent = x ? x->parent : nullptr;
          } else {
            // Case 3: Sibling w's right child is black
            if (!w->right || w->right->color == BLACK) {
              if (w->left)
                w->left->color = BLACK;
              if (w)
                w->color = RED;
              rightRotate(w);
              w = x_parent ? x_parent->right : nullptr;
            }
            // Case 4: Sibling w's right child is red
            if (w)
              w->color = x_parent->color;
            if (x_parent)
              x_parent->color = BLACK;
            if (w && w->right)
              w->right->color = BLACK;
            leftRotate(x_parent);
            x = root;
          }
        } else {
          Node *w = x_parent ? x_parent->left : nullptr;
          // Symmetric cases for right child
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
          }
        }
      }
      if (x)
        x->color = BLACK;
    }

    // Deep copy of the tree starting from node x, with parent p
    Node *copyTree(Node *x, Node *p) {
      if (!x)
        return nullptr;
      Node *new_node = new Node(x->key, p, nullptr, nullptr, x->color);
      new_node->left = copyTree(x->left, new_node);
      new_node->right = copyTree(x->right, new_node);
      return new_node;
    }

  public:
    RBTree() : root(nullptr), node_count(0), comp(Compare()) {}
    ~RBTree() { clear(root); }

    RBTree(const RBTree &other)
        : root(nullptr), node_count(0), comp(other.comp) {
      root = copyTree(other.root, nullptr);
      node_count = other.node_count;
    }

    RBTree &operator=(const RBTree &other) {
      if (this != &other) {
        clear(root);
        root = copyTree(other.root, nullptr);
        node_count = other.node_count;
        comp = other.comp;
      }
      return *this;
    }

    RBTree(RBTree &&other) noexcept
        : root(other.root), node_count(other.node_count),
          comp(std::move(other.comp)) {
      other.root = nullptr;
      other.node_count = 0;
    }

    RBTree &operator=(RBTree &&other) noexcept {
      if (this != &other) {
        clear(root);
        root = other.root;
        node_count = other.node_count;
        comp = std::move(other.comp);
        other.root = nullptr;
        other.node_count = 0;
      }
      return *this;
    }

    // Recursively delete all nodes in the subtree rooted at x
    void clear(Node *x) {
      if (!x)
        return;
      clear(x->left);
      clear(x->right);
      delete x;
    }

    // Return the minimum node in subtree rooted at x
    Node *minimum(Node *x) const {
      while (x && x->left)
        x = x->left;
      return x;
    }

    // Return the maximum node in subtree rooted at x
    Node *maximum(Node *x) const {
      while (x && x->right)
        x = x->right;
      return x;
    }

    // Return the successor of node x in in-order traversal
    Node *successor(Node *x) const {
      if (x->right)
        return minimum(x->right);
      Node *y = x->parent;
      while (y && x == y->right) {
        x = y;
        y = y->parent;
      }
      return y;
    }

    // Return the predecessor of node x in in-order traversal
    Node *predecessor(Node *x) const {
      if (x->left)
        return maximum(x->left);
      Node *y = x->parent;
      while (y && x == y->left) {
        x = y;
        y = y->parent;
      }
      return y;
    }

    // Insert key into the tree, return pair of node and insertion success
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
          return {x, false};
      }

      Node *z = new Node(key, y, nullptr, nullptr, RED);
      if (!y)
        root = z;
      else if (comp(z->key, y->key))
        y->left = z;
      else
        y->right = z;

      insertFixup(z);
      ++node_count;
      return {z, true};
    }

    // Erase node with given key, return number of nodes erased (0 or 1)
    size_t erase(const Key &key) {
      Node *z = root;
      while (z) {
        if (comp(key, z->key))
          z = z->left;
        else if (comp(z->key, key))
          z = z->right;
        else
          break;
      }
      if (!z)
        return 0;

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
        eraseFixup(x, x_parent);

      return 1;
    }

    // Find node with given key or return nullptr
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
    }

    // Find node with smallest key >= given key
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
    }

    // Find node with smallest key > given key
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
    }

    size_t size() const { return node_count; }
    Node *getRoot() const { return root; }
  };

  RBTree tree;

public:
  // Const iterator for ESet, supports in-order traversal
  class const_iterator {
  protected:
    const RBTree *tree;
    Node *node;

  public:
    const_iterator(const RBTree *t = nullptr, Node *n = nullptr)
        : tree(t), node(n) {}

    const Key &operator*() const {
      if (!node)
        throw std::out_of_range("dereferencing end iterator");
      return node->key;
    }

    const_iterator &operator++() {
      if (!node)
        return *this;
      node = tree->successor(node);
      return *this;
    }

    const_iterator operator++(int) {
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    const_iterator &operator--() {
      if (!node) {
        if (!tree || !tree->getRoot())
          return *this;
        node = tree->maximum(tree->getRoot());
      } else {
        Node *pred = tree->predecessor(node);
        if (pred)
          node = pred;
      }
      return *this;
    }

    const_iterator operator--(int) {
      const_iterator tmp = *this;
      --(*this);
      return tmp;
    }

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

  ESet(const ESet &other) : tree(other.tree) {}
  ESet &operator=(const ESet &other) {
    if (this != &other) {
      tree = other.tree;
    }
    return *this;
  }

  ESet(ESet &&other) noexcept : tree(std::move(other.tree)) {}
  ESet &operator=(ESet &&other) noexcept {
    if (this != &other) {
      tree = std::move(other.tree);
    }
    return *this;
  }

  // TODO: Consider adding more public API functions for extensibility

  // Insert element with given arguments, returns iterator and success flag
  template <class... Args> std::pair<iterator, bool> emplace(Args &&...args) {
    Key key(std::forward<Args>(args)...);
    auto [node, inserted] = tree.insert(key);
    return {iterator(&tree, node), inserted};
  }

  // Erase element by key, returns number of elements erased (0 or 1)
  size_t erase(const Key &key) { return tree.erase(key); }

  // Clear all elements from the set
  void clear() {
    tree.clear(tree.root);
    tree.root = nullptr;
    tree.node_count = 0;
  }

  // Find element by key, return iterator to element or end()
  iterator find(const Key &key) const {
    return iterator(&tree, tree.find(key));
  }

  // Count number of elements in range [l, r]
  size_t range(const Key &l, const Key &r) const {
    if (tree.comp(r, l))
      return 0;
    size_t cnt = 0;
    auto it = lower_bound(l);
    auto end_it = upper_bound(r);
    for (; it != end_it; ++it) {
      ++cnt;
    }
    return cnt;
  }

  size_t size() const noexcept { return tree.size(); }

  // Return iterator to first element not less than key
  iterator lower_bound(const Key &key) const {
    return iterator(&tree, tree.lower_bound(key));
  }

  // Return iterator to first element greater than key
  iterator upper_bound(const Key &key) const {
    return iterator(&tree, tree.upper_bound(key));
  }

  // Return iterator to smallest element
  iterator begin() const noexcept {
    return iterator(&tree, tree.minimum(tree.getRoot()));
  }

  // Return iterator to end (past last element)
  iterator end() const noexcept { return iterator(&tree, nullptr); }
};

#endif