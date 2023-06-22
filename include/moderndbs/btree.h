#ifndef INCLUDE_MODERNDBS_BTREE_H
#define INCLUDE_MODERNDBS_BTREE_H

#include <optional>
#include "moderndbs/buffer_manager.h"
#include "moderndbs/segment.h"
#include <vector>
#include <tuple>
#include <bit>
#include <cstring>

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
        static constexpr uint32_t kCapacity = (PageSize-sizeof(uint64_t))/(sizeof(KeyT)+sizeof(uint64_t));

        /// The keys.
        KeyT keys[kCapacity]; // TODO adjust this
        /// The values.
        uint64_t children[kCapacity+1]; // TODO adjust this

        /// Constructor.
        InnerNode() : Node(0, 0) {}
        InnerNode(uint16_t level) : Node(level, 0) {}

        /// Get the index of the first key that is not less than than a provided key.
        /// @param[in] key          The key that should be inserted.
        std::pair<uint32_t, bool> lower_bound(const KeyT &key) {
            // TODO
            if (Node::count-1 <= 0) return {0u, false};
            uint32_t i=0, n = Node::count-1;
            // prepare for power-of-2 binary search
            if (n & (n-1)) {
                n = 1u << (sizeof(n)*8-std::__countl_zero(n)-1);
                i = key > keys[n-1] ? Node::count-1-n : 0;
            }
            // power-of-2 binary search
            for (n/=2; n>0; n/=2) {
                i += key > keys[i+n-1] ? n : 0;
            }
            if (i==Node::count-2u && key > keys[i]) ++i;
            return std::make_pair(i, keys[i] == key);
        }

        /// Insert a key.
        /// @param[in] key          The key that should be inserted.
        /// @param[in] split_page   The child that should be inserted.
        void insert_split(const KeyT &key, uint64_t split_page) {
            // TODO
            if (Node::count-1 == InnerNode::kCapacity) throw std::runtime_error("insert_split(): Not enough space!");
            auto result = lower_bound(key);
            std::memmove(&keys[result.first+1], &keys[result.first], sizeof(KeyT)*(Node::count-1-result.first));
            std::memmove(&children[result.first+1], &children[result.first], sizeof(uint64_t)*(Node::count-result.first));
            keys[result.first] = key;
            children[result.first] = split_page;
            Node::count++;
        }

        /// Split the node.
        /// @param[in] buffer       The buffer for the new page.
        /// @return                 The separator key.
        KeyT split(std::byte* buffer) {
            // HINT
            auto* new_node = new (buffer) InnerNode(Node::level);
            std::memcpy(&new_node->keys[0], &keys[(Node::count+1)/2], sizeof(KeyT)*(Node::count - Node::count/2 - 1));
            std::memcpy(&new_node->children[0], &children[(Node::count+1)/2], sizeof(uint64_t)*(Node::count/2));
            new_node->count = Node::count/2;
            Node::count -= Node::count/2;
            return keys[Node::count-1];
        }

        /// Returns the keys.
        std::vector<KeyT> get_key_vector() {
            // TODO
            return std::vector<KeyT>(&keys[0], &keys[0]+Node::count);
        }
    };

    struct LeafNode: public Node {
        /// The capacity of a node.
        static constexpr uint32_t kCapacity = (PageSize-sizeof(uint64_t))/(sizeof(KeyT)+sizeof(ValueT));

        /// The keys.
        KeyT keys[kCapacity]; // adjust this
        /// The values.
        ValueT values[kCapacity]; // adjust this
        /// The next page
        uint64_t next_page;
        /// Constructor.
        LeafNode() : Node(0, 0), next_page(0) {}

        /// Get the index of the first key that is not less than than a provided key.
        std::pair<uint32_t, bool> lower_bound(const KeyT &key) {
            // TODO
            if (Node::count==0) return std::make_pair(0u, false);
            uint32_t i = 0, n = Node::count;
            // prepare for power-of-2 binary search
            if (n & (n-1)) {
                n = 1u << (sizeof(Node::count)*8-std::__countl_zero(Node::count)-1);
                i = key > keys[n-1] ? Node::count-n : 0;
            }
            // power-of-2 binary search
            for (n/=2; n>0; n/=2) {
                i += key > keys[i+n-1] ? n : 0;
            }
            if (i==Node::count-1u && key > keys[i]) ++i;
            return std::make_pair(i, keys[i] == key);
            }

        /// Insert a key.
        /// @param[in] key          The key that should be inserted.
        /// @param[in] value        The value that should be inserted.
        void insert(const KeyT &key, const ValueT &value) {
            // TODO
            if (Node::count == kCapacity) throw std::runtime_error("Not enough space!");
            auto result = lower_bound(key);
            if (!result.second && result.first < Node::count) {
                std::memmove(&keys[result.first+1], &keys[result.first], sizeof(KeyT)*(Node::count-result.first));
                std::memmove(&values[result.first+1], &values[result.first], sizeof(ValueT)*(Node::count-result.first));
            }
            keys[result.first] = key;
            values[result.first] = value;
            if (!result.second) ++Node::count;
        }

        /// Erase a key.
        void erase(const KeyT &key) {
            // TODO
            auto result = lower_bound(key);
            if (!result.second) return;
            if (result.first < Node::count-1u) {
                std::memmove(&keys[result.first], &keys[result.first+1], sizeof(KeyT)*(Node::count-result.first-1));
                std::memmove(&values[result.first], &values[result.first+1], sizeof(ValueT)*(Node::count-result.first-1));
            }
            --Node::count;
        }

        /// Split the node.
        /// @param[in] buffer       The buffer for the new page.
        /// @return                 The separator key.
        KeyT split(std::byte* buffer) {
            // TODO
            auto* new_node = new (buffer) LeafNode();
            std::memcpy(&new_node->keys[0], &keys[(Node::count+1)/2], sizeof(KeyT)*(Node::count/2));
            std::memcpy(&new_node->values[0], &values[(Node::count+1)/2], sizeof(ValueT)*(Node::count/2));
            new_node->count = Node::count/2;
            Node::count -= Node::count/2;
            return keys[Node::count-1];
        }

        /// Returns the keys.
        std::vector<KeyT> get_key_vector() {
            // TODO
            return std::vector<KeyT>(keys, keys+Node::count);
        }

        /// Returns the values.
        std::vector<ValueT> get_value_vector() {
            // TODO
            return std::vector<ValueT>(values, values+Node::count);
        }
    };

    /// The root.
    uint64_t root; // TODO ensure thread safety of the root node
    /// The next free page;
    uint64_t next_available_page;

    /// Constructor.
    BTree(uint16_t segment_id, BufferManager &buffer_manager)
        : Segment(segment_id, buffer_manager), root(0), next_available_page(1) {
        // TODO
        auto &page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ root, true);
        new (page.get_data()) LeafNode();
        buffer_manager.unfix_page(page, true);
    }

    /// Destructor.
    ~BTree() = default;

    /// Lookup an entry in the tree.
    /// @param[in] key      The key that should be searched.
    /// @return             Whether the key was in the tree.
    std::optional<ValueT> lookup(const KeyT &key) {
        // TODO
        // node <- root node
        // while (node is InnerNode)
        //   node <- node.lookup(key)
        // value <- node.lookup(key)
        auto* page = &buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ root, false);
        auto* node = reinterpret_cast<Node*>(page->get_data());
        while (!node->is_leaf()) {
            auto* inner_node = reinterpret_cast<InnerNode*>(node);
            auto [index, match] = inner_node->lower_bound(key);
            auto child_page_id = inner_node->children[index];
            buffer_manager.unfix_page(*page, false);
            page = &buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ child_page_id, false);
            node = reinterpret_cast<Node*>(page->get_data());
        }
        auto* leaf_node = reinterpret_cast<LeafNode*>(node);
        auto [index, match] = leaf_node->lower_bound(key);
        while (index == leaf_node->count && leaf_node->next_page) {
            auto next_page_id = leaf_node->next_page;
            buffer_manager.unfix_page(*page, false);
            page = &buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ next_page_id, false);
            leaf_node = reinterpret_cast<LeafNode*>(page->get_data());
            std::tie(index, match) = leaf_node->lower_bound(key);
        }
        std::optional<ValueT> result{};
        if (match) result = {leaf_node->values[index]};
        buffer_manager.unfix_page(*page, false);
        return result; 
    }

    /// Erase an entry in the tree.
    /// @param[in] key      The key that should be searched.
    void erase(const KeyT &key) {
        // TODO
        // with underfull pages
        // while node is not leaf:
        //  push page_id to stack
        // erase key
        // if node is not empty and new split key:
        // update parent node and return
        // while stack not empty:
        //  get parent node, index
        //  if parent node has more than one entry
        //      update and return
        //  set count to 0
        // no underfull pages
        // while node is not leaf:
        //  push page_id to stack
        // erase key
        // while node is less than half full:
        //  find neighboor
        //  if neighboor is more than half full:
        //      balance nodes, update separator, return
        //  merge with neighboor, erase separator
        //  node <- parent(node)
        std::vector<std::pair<uint64_t, uint16_t>> stack;
        auto page_id = root;
        auto* page = &buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ page_id, true);
        auto* node = reinterpret_cast<Node*>(page->get_data());
        stack.reserve(node->level);
        while (!node->is_leaf()) {
            auto* inner_node = reinterpret_cast<InnerNode*>(node);
            auto [index, match] = inner_node->lower_bound(key);
            stack.emplace_back(page_id, index);
            page_id = inner_node->children[index];
            buffer_manager.unfix_page(*page, true);
            page = &buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ page_id, true);
            node = reinterpret_cast<Node*>(page->get_data());
        }
        auto* leaf_node = reinterpret_cast<LeafNode*>(node);
        leaf_node->erase(key);
        bool to_delete = leaf_node->count==0; // deletion needed in parent node
        auto split_key = leaf_node->keys[ to_delete ? 0 : leaf_node->count-1u];
        buffer_manager.unfix_page(*page, true);
        while (!stack.empty()) {
            auto [page_id, index] = stack.back();
            stack.pop_back();
            page = &buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ page_id, true);
            auto* inner_node = reinterpret_cast<InnerNode*>(page->get_data());
            if (!to_delete) {
                inner_node->keys[index]=split_key;
                buffer_manager.unfix_page(*page, true);
                return;
            }
            // we have to delete key and child at index
            if (inner_node->count-1u-index>0) {
                std::memmove(&inner_node->keys[index], &inner_node->keys[index+1], sizeof(KeyT)*(inner_node->count-index-1u));
                std::memmove(&inner_node->children[index], &inner_node->children[index+1], sizeof(ValueT)*(inner_node->count-index));
            }
            to_delete = --inner_node->count == 0;
            if (inner_node->count > 1) {
                split_key = inner_node->keys[inner_node->count-2u];
            }
            buffer_manager.unfix_page(*page, true);
        }

        /*
        
        auto* neighboor_page = &buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ leaf_node->next_page, true);
        auto* neighboor_leaf_node = reinterpret_cast<LeafNode*>(neighboor_page->get_data());
        if (neighboor_leaf_node->count+leaf_node->count > LeafNode::kCapacity) {
            // rebalance
            uint16_t move_size = (neighboor_leaf_node->count-leaf_node->count+1u)/2u;
            std::memcpy(&leaf_node->keys[leaf_node->count], &neighboor_leaf_node->keys[0], sizeof(KeyT)*move_size);
            std::memcpy(&leaf_node->values[leaf_node->count], &neighboor_leaf_node->values[0], sizeof(ValueT)*move_size);
            std::memmove(&neighboor_leaf_node->keys[0], &neighboor_leaf_node->keys[move_size], sizeof(KeyT)*(neighboor_leaf_node->count-move_size));
            std::memmove(&neighboor_leaf_node->values[0], &neighboor_leaf_node->values[move_size], sizeof(ValueT)*(neighboor_leaf_node->count-move_size));
            leaf_node->count += move_size;
            neighboor_leaf_node->count -= move_size;
            auto new_key = leaf_node->keys[leaf_node->count-1];
            buffer_manager.unfix_page(*neighboor_page, true);
            buffer_manager.unfix_page(*page, true);
            auto [parent_page_id, index] = stack.back();
            page = &buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ parent_page_id, true);
            auto* parent_node = reinterpret_cast<InnerNode*>(page->get_data());
            parent_node->keys[index] = new_key;
            buffer_manager.unfix_page(*page, true);
            return;
        }
        // erase inner node entry
        std::memcpy(&leaf_node->keys[leaf_node->count], &neighboor_leaf_node->keys[0], sizeof(KeyT)*neighboor_leaf_node->count);
        std::memcpy(&leaf_node->values[leaf_node->count], &neighboor_leaf_node->values[0], sizeof(ValueT)*neighboor_leaf_node->count);
        leaf_node->count += neighboor_leaf_node->count;
        leaf_node->next_page = neighboor_leaf_node->next_page;
        neighboor_leaf_node->count = 0;
        buffer_manager.unfix_page(*neighboor_page, true);
        buffer_manager.unfix_page(*page, true);
        do {
            auto [parent_page_id, index] = stack.back();
            // the position to delete
            ++index;
            stack.pop_back();
            page = &buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ parent_page_id, true);
            auto* inner_node = reinterpret_cast<InnerNode*>(page->get_data());
            // deletion
            if (index < inner_node->count-1) {
                std::memmove(&inner_node->keys[index], &inner_node->keys[index+1], sizeof(KeyT)*inner_node->count-index-1u);
                std::memmove(&inner_node->children[index+1], &inner_node->children[index+2], sizeof(ValueT)*inner_node->count-index-2u);
            }
            inner_node->count--;
            // til here
            if (stack.empty()) {
                // if parent node is root node
                if (inner_node->count == 1) { // if it has only one child
                    inner_node->count = 0;
                    root = page_id;
                }
                buffer_manager.unfix_page(*page, true);
                return;
            }
            if (inner_node->count >= (InnerNode::kCapacity+1)/2) {
                // if the parent node is at least half full
                buffer_manager.unfix_page(*page, true);
                return;
            }
            auto* neighboor_page = &buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ leaf_node->next_page, true);
            auto* neighboor_inner_node = reinterpret_cast<InnerNode*>(neighboor_page->get_data());
            if (neighboor_inner_node->count+inner_node->count > InnerNode::kCapacity) {
                // rebalance
                // get a parent page
                std::tie(parent_page_id, index) = stack.back();
                auto &parent_page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ parent_page_id, true);
                auto* parent_node = reinterpret_cast<InnerNode*>(page->get_data());
                // first insert key from parent
                inner_node->keys[inner_node->count-1] = parent_node->keys[index];
                // then copy n values and n-1 keys from neighboor
                uint16_t move_size = (neighboor_inner_node->count-inner_node->count+1u)/2u;
                std::memcpy(&inner_node->keys[inner_node->count], &neighboor_inner_node->keys[0], sizeof(KeyT)*(move_size-1));
                std::memcpy(&inner_node->values[leaf_node->count], &neighboor_inner_node->values[0], sizeof(ValueT)*move_size);
                // increase inner node count
                inner_node->count += move_size;
                // update parent node key
                parent_node->keys[index] = inner_node->keys[inner_node->count-2u];
                // update neighboor
                std::memmove(&neighboor_inner_node->keys[0], &neighboor_inner_node->keys[move_size], sizeof(KeyT)*(neighboor_leaf_node->count-move_size));
                std::memmove(&neighboor_leaf_node->values[0], &neighboor_leaf_node->values[move_size], sizeof(ValueT)*(neighboor_leaf_node->count-move_size));
                neighboor_leaf_node->count -= move_size;
                buffer_manager.unfix_page(parent_page, true);
                buffer_manager.unfix_page(*neighboor_page, true);
                buffer_manager.unfix_page(*page, true);
                return;
            }
            std::tie(parent_page_id, index) = stack.back();
            auto &parent_page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ parent_page_id, true);
            auto* parent_node = reinterpret_cast<InnerNode*>(page->get_data());
            inner_node->keys[inner_node->count-1] = parent_node->keys[index];
            std::memcpy(&inner_node->keys[inner_node->count], &neighboor_inner_node->keys[0], sizeof(KeyT)*(neighboor_inner_node->count-1));
            std::memcpy(&inner_node->values[inner_node->count], &neighboor_inner_node->values[0], sizeof(ValueT)*neighboor_inner_node->count);
            inner_node->count += neighboor_inner_node->count;
            neighboor_inner_node->count = 0;
            buffer_manager.unfix_page(parent_page, true);
            buffer_manager.unfix_page(*neighboor_page, true);
            buffer_manager.unfix_page(*page, true);
        } while (!stack.empty());
        
        return;*/
        
        /*auto [parent_page_id, index] = stack.back();
        auto& parent_page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ root, false);
        auto* parent_node = reinterpret_cast<InnerNode*>(page->get_data());
        auto neighboor_page_id = parent_node->children[index+1];
        auto& neighboor_page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ root, false);
        while (!stack.empty()) {
            ;
        }
        buffer_manager.unfix_page(*page, false);*/
    }

    /// Inserts a new entry into the tree.
    /// @param[in] key      The key that should be inserted.
    /// @param[in] value    The value that should be inserted.
    void insert(const KeyT &key, const ValueT &value) {
        // TODO
        // if root_node is LeafNode:
        //  if root_node is full:
        //      split root_node to child_1 and child_2
        //      create a new node with references to child_1 and child_2 and declare it as root
        //      insert into appropriate child node
        //      return
        //  else:
        //      insert into root_node
        //      return
        // node <- root_node
        // while node is not LeafNode:
        //  child_node <- node.lookup(key)
        //  if (child_node is full):
        //      split child_node 
        //      update keys and children of node accordingly
        //      child_node <- node.lookup(key)
        //  node <- child_node
        // insert into node
        auto* page = &buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ root, true);
        auto* node = reinterpret_cast<Node*>(page->get_data());
        if (node->is_leaf()) {
            // initialize the root if needed
            auto* leaf_node = node->count ? static_cast<LeafNode*>(node) : new (node) LeafNode();
            if (leaf_node->count == LeafNode::kCapacity) { // if the root is full
                // make a copy
                auto &child_page_1 = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ next_available_page++, true);
                std::memcpy(child_page_1.get_data(), page->get_data(), PageSize);
                leaf_node = reinterpret_cast<LeafNode*>(child_page_1.get_data());
                // create a new child node
                auto &child_page_2 = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ next_available_page++, true);
                // split
                auto split_key = leaf_node->split(reinterpret_cast<std::byte*>(child_page_2.get_data()));
                leaf_node->next_page = next_available_page-1u;
                // depending on key, insert should be done either on original root (now left child) or newly created child
                if (key > split_key) {
                    leaf_node = reinterpret_cast<LeafNode*>(child_page_2.get_data());
                }
                // insertion
                leaf_node->insert(key, value);
                // release children pages
                buffer_manager.unfix_page(child_page_2, true);
                buffer_manager.unfix_page(child_page_1, true);
                // setup new root node
                auto* new_root_node = new (page->get_data()) InnerNode(1);
                new_root_node->children[0] = next_available_page-1u;
                new_root_node->count = 1;
                new_root_node->insert_split(split_key, next_available_page-2u);
            }
            else { // not yet full; simple insert
                leaf_node->insert(key, value);
            }
            buffer_manager.unfix_page(*page, true);
            return;
        }
        else { // basically the same as if it was a leaf node
            auto* inner_node = static_cast<InnerNode*>(node);
            if (inner_node->count-1 == InnerNode::kCapacity) {
                auto &child_page_1 = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ next_available_page++, true);
                std::memcpy(child_page_1.get_data(), page->get_data(), PageSize);
                inner_node = reinterpret_cast<InnerNode*>(child_page_1.get_data());
                auto &child_page_2 = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ next_available_page++, true);
                auto split_key = inner_node->split(reinterpret_cast<std::byte*>(child_page_2.get_data()));
                auto* new_root_node = new (page->get_data()) InnerNode(node->level+1);
                new_root_node->children[0] = next_available_page-1u;
                new_root_node->count = 1;
                new_root_node->insert_split(split_key, next_available_page-2u);
                buffer_manager.unfix_page(*page, true);
                if (key > split_key) {
                    node = reinterpret_cast<Node*>(child_page_2.get_data());
                    page = &child_page_2;
                    buffer_manager.unfix_page(child_page_1, true);
                }
                else {
                    node = reinterpret_cast<Node*>(child_page_1.get_data());
                    page = &child_page_1;
                    buffer_manager.unfix_page(child_page_2, true);
                }
            }
        }
        auto* inner_node = static_cast<InnerNode*>(node);
        while (true) {
            auto [index, match] = inner_node->lower_bound(key);
            auto child_page_id = inner_node->children[index];
            auto* child_page = &buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ child_page_id, true);
            auto* child_node = reinterpret_cast<Node*>(child_page->get_data());
            if (child_node->is_leaf()) {
                auto* child_leaf_node = static_cast<LeafNode*>(child_node);
                if (child_node->count == LeafNode::kCapacity) {
                    auto &new_child_page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ next_available_page++, true);
                    auto split_key = child_leaf_node->split(reinterpret_cast<std::byte*>(new_child_page.get_data()));
                    child_leaf_node->next_page = next_available_page-1u;
                    if (key > split_key) {
                        child_leaf_node = reinterpret_cast<LeafNode*>(new_child_page.get_data());
                    }
                    child_leaf_node->insert(key, value);
                    buffer_manager.unfix_page(new_child_page, true);
                    buffer_manager.unfix_page(*child_page, true);
                    inner_node->children[index]=next_available_page-1u;
                    inner_node->insert_split(split_key, child_page_id);
                    buffer_manager.unfix_page(*page, true);
                }
                else {
                    child_leaf_node->insert(key, value);
                    buffer_manager.unfix_page(*child_page, true);
                    buffer_manager.unfix_page(*page, true);
                }
                return;
            }
            else {
                auto* child_inner_node = static_cast<InnerNode*>(child_node);
                if (child_node->count-1 == InnerNode::kCapacity) {
                    auto &new_child_page = buffer_manager.fix_page((static_cast<uint64_t>(segment_id) << 48) ^ next_available_page++, true);
                    auto split_key = child_inner_node->split(reinterpret_cast<std::byte*>(new_child_page.get_data()));
                    if (key > split_key) {
                        child_inner_node = reinterpret_cast<InnerNode*>(new_child_page.get_data());
                        buffer_manager.unfix_page(*child_page, true);
                        child_page = &new_child_page;
                    }
                    else {
                        buffer_manager.unfix_page(new_child_page, true);
                    }
                    inner_node->children[index] = next_available_page-1u;
                    inner_node->insert_split(split_key, child_page_id);
                }
                buffer_manager.unfix_page(*page, true);
                page = child_page;
                inner_node = child_inner_node;
            }
        }
    }
};

}  // namespace moderndbs

#endif
