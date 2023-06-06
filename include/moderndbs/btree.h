#ifndef INCLUDE_MODERNDBS_BTREE_H
#define INCLUDE_MODERNDBS_BTREE_H

#include <optional>
#include "moderndbs/buffer_manager.h"
#include "moderndbs/segment.h"

namespace moderndbs {

template<typename KeyT, typename ValueT, typename ComparatorT, size_t PageSize>
struct BTree : public Segment {
    struct Node {
        /// The level in the tree.
        uint16_t level;
        /// The number of children.
        uint16_t count;

        // Constructor
        Node(uint16_t level, uint16_t count)
            : level(level), count(count) {}

        /// Is the node a leaf node?
        bool is_leaf() const { return level == 0; }
    };

    struct InnerNode: public Node {
        /// The capacity of a node.
        /// TODO think about the capacity that the nodes have.
        static constexpr uint32_t kCapacity = 42;

        /// The keys.
        KeyT keys[kCapacity]; // TODO adjust this
        /// The values.
        uint64_t children[kCapacity]; // TODO adjust this

        /// Constructor.
        InnerNode() : Node(0, 0) {}

        /// Get the index of the first key that is not less than than a provided key.
        /// @param[in] key          The key that should be inserted.
        std::pair<uint32_t, bool> lower_bound(const KeyT &key) {
            // TODO
            (void)key;
        }

        /// Insert a key.
        /// @param[in] key          The key that should be inserted.
        /// @param[in] split_page   The child that should be inserted.
        void insert_split(const KeyT &key, uint64_t split_page) {
            // TODO
            (void)key;
            (void)split_page;
        }

        /// Split the node.
        /// @param[in] buffer       The buffer for the new page.
        /// @return                 The separator key.
        KeyT split(std::byte* buffer) {
            // HINT
            (void)buffer;
        }

        /// Returns the keys.
        std::vector<KeyT> get_key_vector() {
            // TODO
        }
    };

    struct LeafNode: public Node {
        /// The capacity of a node.
        static constexpr uint32_t kCapacity = 42;

        /// The keys.
        KeyT keys[kCapacity]; // adjust this
        /// The values.
        ValueT values[kCapacity]; // adjust this

        /// Constructor.
        LeafNode() : Node(0, 0) {}

        /// Get the index of the first key that is not less than than a provided key.
        std::pair<uint32_t, bool> lower_bound(const KeyT &key) {
            (void)key;
            // TODO
        }

        /// Insert a key.
        /// @param[in] key          The key that should be inserted.
        /// @param[in] value        The value that should be inserted.
        void insert(const KeyT &key, const ValueT &value) {
            (void)key;
            (void)value;
            // TODO
        }

        /// Erase a key.
        void erase(const KeyT &key) {
            (void)key;
            // TODO
        }

        /// Split the node.
        /// @param[in] buffer       The buffer for the new page.
        /// @return                 The separator key.
        KeyT split(std::byte* buffer) {
            (void)buffer;
            // TODO
        }

        /// Returns the keys.
        std::vector<KeyT> get_key_vector() {
            // TODO
        }

        /// Returns the values.
        std::vector<ValueT> get_value_vector() {
            // TODO
        }
    };

    /// The root.
    uint64_t root; // TODO ensure thread safety of the root node

    /// Constructor.
    BTree(uint16_t segment_id, BufferManager &buffer_manager)
        : Segment(segment_id, buffer_manager) {
        // TODO
    }

    /// Destructor.
    ~BTree() = default;

    /// Lookup an entry in the tree.
    /// @param[in] key      The key that should be searched.
    /// @return             Whether the key was in the tree.
    std::optional<ValueT> lookup(const KeyT &key) {
        (void)key;
        // TODO
    }

    /// Erase an entry in the tree.
    /// @param[in] key      The key that should be searched.
    void erase(const KeyT &key) {
        (void)key;
        // TODO
    }

    /// Inserts a new entry into the tree.
    /// @param[in] key      The key that should be inserted.
    /// @param[in] value    The value that should be inserted.
    void insert(const KeyT &key, const ValueT &value) {
        (void)key;
        (void)value;
        // TODO
    }
};

}  // namespace moderndbs

#endif
