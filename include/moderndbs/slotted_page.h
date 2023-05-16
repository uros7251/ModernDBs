#ifndef INCLUDE_MODERNDBS_SLOTTED_PAGE_H_
#define INCLUDE_MODERNDBS_SLOTTED_PAGE_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace moderndbs {

struct TID {
    public:
    /// Constructor.
    explicit TID(uint64_t raw_value)
        : value(raw_value) {}
    /// Constructor.
    TID(uint64_t page, uint16_t slot)
        : value((page << 16) ^ (slot & 0xFFFF)) {
    }

    /// Get buffer page id.
    [[nodiscard]] uint64_t get_page_id(uint16_t segment_id) const {
        return (value >> 16) ^ (static_cast<uint64_t>(segment_id) << 48);
    }
    /// Get the slot.
    [[nodiscard]] uint16_t get_slot() const { return value & 0xFFFF; }
    /// Get the value.
    [[nodiscard]] uint64_t get_value() const { return value; }

    private:
    /// The TID value.
    uint64_t value;
};

struct SlottedPage {
    struct Header {
        /// Constructor
        explicit Header(uint32_t page_size);

        /// Number of currently used slots.
        uint16_t slot_count;
        /// To speed up the search for a free slot.
        uint16_t first_free_slot;
        /// Lower end of the data.
        uint32_t data_start;
        /// Space that would be available after compactification.
        uint32_t free_space;
    };

    struct Slot {
        /// Constructor
        Slot() = default;

        /// Is redirect?
        [[nodiscard]] bool is_redirect() const { return (value >> 56) != 0xFF; }
        /// Is redirect target?
        [[nodiscard]] bool is_redirect_target() const { return ((value >> 48) & 0xFF) != 0; }
        /// Reinterpret slot as TID.
        [[nodiscard]] TID as_redirect_tid() const { return TID(value); }
        /// Clear the slot.
        void clear() { value = 0; }
        /// Get the size.
        [[nodiscard]] uint32_t get_size() const { return value & 0xFFFFFFull; }
        /// Get the offset.
        [[nodiscard]] uint32_t get_offset() const { return (value >> 24) & 0xFFFFFFull; }

        /// Is empty?
        [[nodiscard]] bool is_empty() const { return value == 0; }

        /// Set the slot.
        void set_slot(uint32_t offset, uint32_t size, bool is_redirect_target) {
            value = 0;
            value ^= size & 0xFFFFFFull;
            value ^= (offset & 0xFFFFFFull) << 24;
            value ^= (is_redirect_target ? 0xFFull : 0x00ull) << 48;
            value ^= 0xFFull << 56;
        }

        /// Set the redirect.
        void set_redirect_tid(TID tid) {
            assert((0xFF - (tid.get_value() >> 56)) > 0 && "invalid tid");
            value = tid.get_value();
        }

        /// Mark a slot as redirect target.
        void mark_as_redirect_target(bool redirect = true) {
            value &= ~(0xFFull << 48);
            if (redirect) {
                value ^= 0xFFull << 48;
            }
        }

        /// The slot value.
        uint64_t value{0};
    };

    /// Constructor.
    /// @param[in] page_size    The size of a buffer frame.
    explicit SlottedPage(uint32_t page_size);

    /// Get data.
    std::byte *get_data();
    /// Get constant data.
    [[nodiscard]] const std::byte *get_data() const;

    /// Get slots.
    Slot *get_slots();
    /// Get constant slots.
    [[nodiscard]] const Slot *get_slots() const;

    /// Get the compacted free space.
    [[nodiscard]] uint32_t get_free_space() const { return header.free_space; }

    /// Get the fragmented free space.
    uint32_t get_fragmented_free_space();

    // Allocate a slot.
    /// @param[in] data_size    The slot that should be allocated
    /// @param[in] page_size    The new size of a slot
    uint16_t allocate(uint32_t data_size, uint32_t page_size);

    // Relocate a slot.
    /// @param[in] slot_id      The slot that should be relocated
    /// @param[in] data_size    The new size of a slot
    /// @param[in] page_size    The size of the page
    void relocate(uint16_t slot_id, uint32_t data_size, uint32_t page_size);

    /// Erase a slot.
    /// @param[in] slot_id      The slot that should be erased
    void erase(uint16_t slot_id);

    /// Compact the page.
    /// @param[in] page_size    The size of a buffer frame.
    void compactify(uint32_t page_size);

    /// The header.
    /// Note that the slotted page itself should reside on the buffer frame!
    /// DO NOT allocate heap objects for a slotted page but instead reinterpret_cast BufferFrame.get_data()!
    /// This is also the reason why the constructor and compactify require the actual page size as argument.
    /// (The slotted page itself does not know how large it is)
    Header header;
};

static_assert(sizeof(SlottedPage) == sizeof(SlottedPage::Header), "An empty slotted page must only contain the header");

}  // namespace moderndbs

#endif // INCLUDE_MODERNDBS_SLOTTED_PAGE_H_
