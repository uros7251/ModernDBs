#ifndef INCLUDE_MODERNDBS_BUFFER_MANAGER_H
#define INCLUDE_MODERNDBS_BUFFER_MANAGER_H

#include <cstddef>
#include <cstdint>
#include <exception>
#include <vector>
#include <list>
#include <forward_list>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include "moderndbs/file.h"


namespace moderndbs {

class PageIO {
private:
    std::mutex mtx;
    std::unordered_map<uint64_t, size_t> offset;
    std::unordered_map<uint16_t, size_t> last_offset;
    std::unordered_map<uint16_t, std::unique_ptr<File>> segment_file_handle;
    PageIO() { } // constructor is private!
public:
    bool read_page(uint64_t page_id, char* buffer, size_t size);
    void write_page(uint64_t page_id, char* buffer, size_t size);

    static std::shared_ptr<PageIO> get_instance();
};

class BufferFrame {
private:
    friend class BufferManager;
    // TODO: add your implementation here
    uint64_t page_id;
    std::shared_mutex latch;
    int pin_count;
    // you are allowed to modify the following fields only while you hold a lock
    bool exclusive;
    bool dirty;
    char* data;
public:
    BufferFrame(uint64_t page_ID, bool excl, char* data_ptr)
        :page_id(page_ID), pin_count(1), exclusive(excl), dirty(false), data(data_ptr) { }
    
    BufferFrame(const BufferFrame& other)
        :BufferFrame(other.page_id, other.exclusive, other.data) { dirty = other.dirty; }
    
    /// Returns a pointer to this page's data.
    char* get_data() { return data; }
};


class buffer_full_error
: public std::exception {
public:
    [[nodiscard]] const char* what() const noexcept override {
        return "buffer is full";
    }
};


class BufferManager {
    struct IterWrapper {
        bool in_fifo;
        std::list<BufferFrame>::iterator it;
        IterWrapper(bool fifo, std::list<BufferFrame>::iterator iter)
            :in_fifo(fifo), it(iter) {}
        IterWrapper() {}
    };
private:
    // TODO: add your implementation here
    std::mutex mtx;

    size_t _page_size;
    size_t _page_count;
    char* _buffer;

    std::list<BufferFrame> _fifo;
    std::list<BufferFrame> _lru;

    std::unordered_map<uint64_t, BufferManager::IterWrapper> _map;
    std::unordered_map<uint16_t, std::unique_ptr<File>> _file_handles;

    std::shared_ptr<PageIO> _page_io;

    void flush_frame(BufferFrame&);
    void read_page(BufferFrame&);
    File& get_segment_file(uint64_t);
public:
   BufferManager(const BufferManager&) = delete;
   BufferManager(BufferManager&&) = delete;
   BufferManager& operator=(const BufferManager&) = delete;
   BufferManager& operator=(BufferManager&&) = delete;
    /// Constructor.
    /// @param[in] page_size  Size in bytes that all pages will have.
    /// @param[in] page_count Maximum number of pages that should reside in
    //                        memory at the same time.
    BufferManager(size_t page_size, size_t page_count);

    /// Destructor. Writes all dirty pages to disk.
    ~BufferManager();

    /// Returns a reference to a `BufferFrame` object for a given page id. When
    /// the page is not loaded into memory, it is read from disk. Otherwise the
    /// loaded page is used.
    /// When the page cannot be loaded because the buffer is full, throws the
    /// exception `buffer_full_error`.
    /// Is thread-safe w.r.t. other concurrent calls to `fix_page()` and
    /// `unfix_page()`.
    /// @param[in] page_id   Page id of the page that should be loaded.
    /// @param[in] exclusive If `exclusive` is true, the page is locked
    ///                      exclusively. Otherwise it is locked
    ///                      non-exclusively (shared).
    BufferFrame& fix_page(uint64_t page_id, bool exclusive);

    /// Takes a `BufferFrame` reference that was returned by an earlier call to
    /// `fix_page()` and unfixes it. When `is_dirty` is / true, the page is
    /// written back to disk eventually.
    void unfix_page(BufferFrame& page, bool is_dirty);

    /// Returns the page ids of all pages (fixed and unfixed) that are in the
    /// FIFO list in FIFO order.
    /// Is not thread-safe.
    [[nodiscard]] std::vector<uint64_t> get_fifo_list() const;

    /// Returns the page ids of all pages (fixed and unfixed) that are in the
    /// LRU list in LRU order.
    /// Is not thread-safe.
    [[nodiscard]] std::vector<uint64_t> get_lru_list() const;

    /// Returns the segment id for a given page id which is contained in the 16
    /// most significant bits of the page id.
    static constexpr uint16_t get_segment_id(uint64_t page_id) {
        return page_id >> 48;
    }

    /// Returns the page id within its segment for a given page id. This
    /// corresponds to the 48 least significant bits of the page id.
    static constexpr uint64_t get_segment_page_id(uint64_t page_id) {
        return page_id & ((1ull << 48) - 1);
    }
};


}  // namespace moderndbs

#endif
