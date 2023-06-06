#include <gtest/gtest.h>
#include <algorithm>
#include <cstddef>
#include <barrier>
#include <numeric>
#include <random>
#include <sstream>
#include <thread>
#include <vector>
#include "moderndbs/defer.h"
#include "moderndbs/btree.h"

using BufferFrame = moderndbs::BufferFrame;
using BufferManager = moderndbs::BufferManager;
using Defer = moderndbs::Defer;
using BTree = moderndbs::BTree<uint64_t, uint64_t, std::less<uint64_t>, 1024>;

namespace {

// NOLINTNEXTLINE
TEST(BTreeTest, Capacity) {
   // Here, 42 is not the answer...
   ASSERT_NE(BTree::InnerNode::kCapacity, 42);
   ASSERT_NE(BTree::LeafNode::kCapacity, 42);

   ASSERT_LE(1000, sizeof(BTree::InnerNode));
   ASSERT_LE(sizeof(BTree::InnerNode), 1024);
   
   ASSERT_LE(1000, sizeof(BTree::LeafNode));
   ASSERT_LE(sizeof(BTree::LeafNode), 1024);
   
   // Larger buffer pages allow nodes with higher fanout
   using BTree = moderndbs::BTree<uint64_t, uint64_t, std::less<uint64_t>, 1u << 16>;
   ASSERT_LE(64000, sizeof(BTree::InnerNode));
   ASSERT_LE(sizeof(BTree::InnerNode), 1u << 16);
   
   ASSERT_LE(64000, sizeof(BTree::LeafNode));
   ASSERT_LE(sizeof(BTree::LeafNode), 1u << 16);
}

// NOLINTNEXTLINE
TEST(BTreeTest, LeafNodeInsert) {
    std::vector<std::byte> buffer;
    buffer.resize(1024);
    auto node = new (buffer.data()) BTree::LeafNode();
    ASSERT_EQ(node->count, 0);

    std::stringstream expected_keys;
    std::stringstream expected_values;
    std::stringstream seen_keys;
    std::stringstream seen_values;

    auto n = BTree::LeafNode::kCapacity;

    // Insert children into the leaf nodes
    for (uint32_t i = 0, j = 0; i < n; ++i, j = i * 2) {
        node->insert(i, j);

        ASSERT_EQ(node->count, i + 1)
            << "LeafNode::insert did not increase the the child count";

        // Append current node to expected nodes
        expected_keys << ((i != 0) ? ", " : "") << i;
        expected_values << ((i != 0) ? ", " : "") << j;
    }

    // Check the number of keys & children
    auto keys = node->get_key_vector();
    auto values = node->get_value_vector();

    ASSERT_EQ(keys.size(), n)
        << "leaf node must contain " << n << " keys for " << n << " values";
    ASSERT_EQ(values.size(), n)
        << "leaf node must contain " << n << " values";

    // Check the keys
    for (uint32_t i = 0; i < n; ++i) {
        auto k = keys[i];
        seen_keys << ((i != 0) ? ", " : "") << k;
        ASSERT_EQ(k, i)
            << "leaf node does not store key=" << i << "\n"
            << "EXPECTED:\n"
            << expected_keys.str()
            << "\nSEEN:\n"
            << seen_keys.str();
    }

    // Check the children
    for (uint32_t i = 0, j = 0; i < n; ++i, j = i * 2) {
        auto c = values[i];
        seen_values << ((i != 0) ? ", " : "") << c;
        ASSERT_EQ(c, j)
            << "leaf node does not store value=" << j << "\n"
            << "EXPECTED:\n"
            << expected_values.str()
            << "\nSEEN:\n"
            << seen_values.str();
    }
}

// NOLINTNEXTLINE
TEST(BTreeTest, LeafNodeSplit) {
    std::vector<std::byte> buffer_left;
    std::vector<std::byte> buffer_right;
    buffer_left.resize(1024);
    buffer_right.resize(1024);

    auto left_node = new (buffer_left.data()) BTree::LeafNode();
    auto right_node = reinterpret_cast<BTree::LeafNode*>(buffer_right.data());
    ASSERT_EQ(left_node->count, 0);

    auto n = BTree::LeafNode::kCapacity;

    // Fill the left node
    for (uint32_t i = 0, j = 0; i < n; ++i, j = i * 2) {
        left_node->insert(i, j);
    }

    // Check the number of keys & children
    auto left_keys = left_node->get_key_vector();
    auto left_values = left_node->get_value_vector();
    ASSERT_EQ(left_keys.size(), n);
    ASSERT_EQ(left_values.size(), n);

    // Now split the left node
    auto separator = left_node->split(buffer_right.data());
    ASSERT_EQ(left_node->count, n / 2 + 1);
    ASSERT_EQ(right_node->count, n - (n / 2) - 1);
    ASSERT_EQ(separator, n / 2);

    // Check keys & children of the left node
    left_keys = left_node->get_key_vector();
    left_values = left_node->get_value_vector();
    ASSERT_EQ(left_keys.size(), left_node->count);
    ASSERT_EQ(left_values.size(), left_node->count);
    for (auto i = 0; i < left_node->count - 1; ++i) {
        ASSERT_EQ(left_keys[i], i);
    }
    for (auto i = 0; i < left_node->count; ++i) {
        ASSERT_EQ(left_values[i], i * 2);
    }

    // Check keys & children of the right node
    auto right_keys = right_node->get_key_vector();
    auto right_values = right_node->get_value_vector();
    ASSERT_EQ(right_keys.size(), right_node->count);
    ASSERT_EQ(right_values.size(), right_node->count);
    for (auto i = 0; i < right_node->count; ++i) {
        ASSERT_EQ(right_keys[i], left_node->count + i);
    }
    for (auto i = 0; i < right_node->count; ++i) {
        ASSERT_EQ(right_values[i], (left_node->count + i) * 2);
    }
}

// NOLINTNEXTLINE
TEST(BTreeTest, InsertEmptyTree) {
    BufferManager buffer_manager(1024, 100);
    BTree tree(0, buffer_manager);
    tree.insert(42, 21);

    auto test = "inserting an element into an empty B-Tree";
    auto& root_page = buffer_manager.fix_page(tree.root, false);
    auto root_node = reinterpret_cast<BTree::Node*>(root_page.get_data());
    Defer root_page_unfix([&]() { buffer_manager.unfix_page(root_page, false); });

    ASSERT_TRUE(root_node->is_leaf())
        << test << " does not create a leaf node.";
    ASSERT_TRUE(root_node->count)
        << test << " does not create a leaf node with count = 1.";
}

// NOLINTNEXTLINE
TEST(BTreeTest, InsertLeafNode) {
    uint32_t page_size = 1024;
    BufferManager buffer_manager(page_size, 100);
    BTree tree(0, buffer_manager);

    for (auto i = 0ul; i < BTree::LeafNode::kCapacity; ++i) {
        tree.insert(i, 2 * i);
    }

    auto test = "inserting BTree::LeafNode::kCapacity elements into an empty B-Tree";

    auto& root_page = buffer_manager.fix_page(tree.root, false);
    auto root_node = reinterpret_cast<BTree::Node*>(root_page.get_data());
    auto root_inner_node = static_cast<BTree::InnerNode*>(root_node);
    Defer root_page_unfix([&]() { buffer_manager.unfix_page(root_page, false); });

    ASSERT_TRUE(root_node->is_leaf())
        << test << " creates an inner node as root.";
    ASSERT_EQ(root_inner_node->count, BTree::LeafNode::kCapacity)
        << test << " does not store all elements.";
}

// NOLINTNEXTLINE
TEST(BTreeTest, InsertLeafNodeSplit) {
    BufferManager buffer_manager(1024, 100);
    BTree tree(0, buffer_manager);

    for (auto i = 0ul; i < BTree::LeafNode::kCapacity; ++i) {
        tree.insert(i, 2 * i);
    }

    auto* root_page = &buffer_manager.fix_page(tree.root, false);
    auto root_node = reinterpret_cast<BTree::Node*>(root_page->get_data());
    auto root_inner_node = static_cast<BTree::InnerNode*>(root_node);
    Defer root_page_unfix([&]() { buffer_manager.unfix_page(*root_page, false); });
    ASSERT_TRUE(root_inner_node->is_leaf());
    ASSERT_EQ(root_inner_node->count, BTree::LeafNode::kCapacity);
    root_page_unfix.run();

    // Let there be a split...
    tree.insert(424242, 42);

    auto test = "inserting BTree::LeafNode::kCapacity + 1 elements into an empty B-Tree";

    root_page = &buffer_manager.fix_page(tree.root, false);
    root_node = reinterpret_cast<BTree::Node*>(root_page->get_data());
    root_inner_node = static_cast<BTree::InnerNode*>(root_node);
    root_page_unfix = Defer([&]() { buffer_manager.unfix_page(*root_page, false); });

    ASSERT_FALSE(root_inner_node->is_leaf())
        << test << " does not create a root inner node";
    ASSERT_EQ(root_inner_node->count, 2)
        << test << " creates a new root with count != 2";
}

// NOLINTNEXTLINE
TEST(BTreeTest, LookupEmptyTree) {
    BufferManager buffer_manager(1024, 100);
    BTree tree(0, buffer_manager);

    auto test = "searching for a non-existing element in an empty B-Tree";

    ASSERT_FALSE(tree.lookup(42))
        << test << " seems to return something :-O";
}

// NOLINTNEXTLINE
TEST(BTreeTest, LookupSingleLeaf) {
    BufferManager buffer_manager(1024, 100);
    BTree tree(0, buffer_manager);

    // Fill one page
    for (auto i = 0ul; i < BTree::LeafNode::kCapacity; ++i) {
        tree.insert(i, 2 * i);
        ASSERT_TRUE(tree.lookup(i))
            << "searching for the just inserted key k=" << i << " yields nothing";
    }

    // Lookup all values
    for (auto i = 0ul; i < BTree::LeafNode::kCapacity; ++i) {
        auto v = tree.lookup(i);
        ASSERT_TRUE(v)
            << "key=" << i << " is missing";
        ASSERT_EQ(*v, 2* i)
            << "key=" << i << " should have the value v=" << 2*i;
    }
}

// NOLINTNEXTLINE
TEST(BTreeTest, LookupSingleSplit) {
    BufferManager buffer_manager(1024, 100);
    BTree tree(0, buffer_manager);

    // Insert values
    for (auto i = 0ul; i < BTree::LeafNode::kCapacity; ++i) {
        tree.insert(i, 2 * i);
    }

    tree.insert(BTree::LeafNode::kCapacity, 2 * BTree::LeafNode::kCapacity);
    ASSERT_TRUE(tree.lookup(BTree::LeafNode::kCapacity))
        << "searching for the just inserted key k=" << (BTree::LeafNode::kCapacity + 1) << " yields nothing";

    // Lookup all values
    for (auto i = 0ul; i < BTree::LeafNode::kCapacity + 1; ++i) {
        auto v = tree.lookup(i);
        ASSERT_TRUE(v)
            << "key=" << i << " is missing";
        ASSERT_EQ(*v, 2* i)
            << "key=" << i << " should have the value v=" << 2*i;
    }
}

// NOLINTNEXTLINE
TEST(BTreeTest, LookupMultipleSplitsIncreasing) {
    BufferManager buffer_manager(1024, 100);
    BTree tree(0, buffer_manager);
    auto n = 100 * BTree::LeafNode::kCapacity;

    // Insert values
    for (auto i = 0ul; i < n; ++i) {
        tree.insert(i, 2 * i);
        ASSERT_TRUE(tree.lookup(i))
            << "searching for the just inserted key k=" << i << " yields nothing";
    }

    // Lookup all values
    for (auto i = 0ul; i < n; ++i) {
        auto v = tree.lookup(i);
        ASSERT_TRUE(v)
            << "key=" << i << " is missing";
        ASSERT_EQ(*v, 2* i)
            << "key=" << i << " should have the value v=" << 2*i;
    }
}

// NOLINTNEXTLINE
TEST(BTreeTest, LookupMultipleSplitsDecreasing) {
    BufferManager buffer_manager(1024, 100);
    BTree tree(0, buffer_manager);
    auto n = 10 * BTree::LeafNode::kCapacity;

    // Insert values
    for (auto i = n; i > 0; --i) {
        tree.insert(i, 2 * i);
        ASSERT_TRUE(tree.lookup(i))
            << "searching for the just inserted key k=" << i << " yields nothing";
    }

    // Lookup all values
    for (auto i = n; i > 0; --i) {
        auto v = tree.lookup(i);
        ASSERT_TRUE(v)
            << "key=" << i << " is missing";
        ASSERT_EQ(*v, 2* i)
            << "key=" << i << " should have the value v=" << 2*i;
    }
}

// NOLINTNEXTLINE
TEST(BTreeTest, LookupRandomNonRepeating) {
    BufferManager buffer_manager(1024, 100);
    BTree tree(0, buffer_manager);
    auto n = 10 * BTree::LeafNode::kCapacity;

    // Generate random non-repeating key sequence
    std::vector<uint64_t> keys(n);
    std::iota(keys.begin(), keys.end(), n);
    std::mt19937_64 engine(0);
    std::shuffle(keys.begin(), keys.end(), engine);

    // Insert values
    for (auto i = 0ul; i < n; ++i) {
        tree.insert(keys[i], 2 * keys[i]);
        ASSERT_TRUE(tree.lookup(keys[i]))
            << "searching for the just inserted key k=" << keys[i] << " after i=" << i << " inserts yields nothing";
    }

    // Lookup all values
    for (auto i = 0ul; i < n; ++i) {
        auto v = tree.lookup(keys[i]);
        ASSERT_TRUE(v)
            << "key=" << keys[i] << " is missing";
        ASSERT_EQ(*v, 2 * keys[i])
            << "key=" << keys[i] << " should have the value v=" << keys[i];
    }
}

// NOLINTNEXTLINE
TEST(BTreeTest, LookupRandomRepeating) {
    BufferManager buffer_manager(1024, 100);
    BTree tree(0, buffer_manager);
    auto n = 10 * BTree::LeafNode::kCapacity;

    // Insert & updated 100 keys at random
    std::mt19937_64 engine{0};
    std::uniform_int_distribution<uint64_t> key_distr(0, 99);
    std::vector<uint64_t> values(100);

    for (auto i = 1ul; i < n; ++i) {
        uint64_t rand_key = key_distr(engine);
        values[rand_key] = i;
        tree.insert(rand_key, i);

        auto v = tree.lookup(rand_key);
        ASSERT_TRUE(v)
            << "searching for the just inserted key k=" << rand_key << " after i=" << (i - 1) << " inserts yields nothing";
        ASSERT_EQ(*v, i)
            << "overwriting k=" << rand_key << " with value v=" << i << " failed";
    }

    // Lookup all values
    for (auto i = 0ul; i < 100; ++i) {
        if (values[i] == 0) {
            continue;
        }
        auto v = tree.lookup(i);
        ASSERT_TRUE(v)
            << "key=" << i << " is missing";
        ASSERT_EQ(*v, values[i])
            << "key=" << i << " should have the value v=" << values[i];
    }
}

// NOLINTNEXTLINE
TEST(BTreeTest, Erase) {
    BufferManager buffer_manager(1024, 100);
    BTree tree(0, buffer_manager);

    // Insert values
    for (auto i = 0ul; i < 2 * BTree::LeafNode::kCapacity; ++i) {
        tree.insert(i, 2 * i);
    }

    // Iteratively erase all values
    for (auto i = 0ul; i < 2 * BTree::LeafNode::kCapacity; ++i) {
        ASSERT_TRUE(tree.lookup(i))
            << "k=" << i << " was not in the tree";
        tree.erase(i);
        ASSERT_FALSE(tree.lookup(i))
            << "k=" << i << " was not removed from the tree";
    }
}

TEST(BTreeTest, MultithreadWriters) {
   BufferManager buffer_manager(1024, 100);
   BTree tree(0, buffer_manager);
   
   std::barrier sync_point(4);
   std::vector<std::thread> threads;
   for (size_t thread = 0; thread < 4; ++thread) {
      threads.emplace_back([thread, &sync_point, &tree] {
         size_t startValue = thread * 2 * BTree::LeafNode::kCapacity;
         size_t limit = startValue + 2 * BTree::LeafNode::kCapacity;
         // Insert values
         for (auto i = startValue; i < limit; ++i) {
            tree.insert(i, 2 * i);
         }
         
         sync_point.arrive_and_wait();
         
         // And read them back
         for (auto i = startValue; i < limit; ++i) {
            auto res = tree.lookup(i);
            ASSERT_TRUE(res);
            ASSERT_TRUE(*res == 2 * i);
         }
      });
   }
   for (auto& t : threads)
      t.join();
}

}  // namespace
