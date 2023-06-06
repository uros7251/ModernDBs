#include "moderndbs/buffer_manager.h"
#include <cassert>

/*
This is only a dummy implementation of a buffer manager. It does not do any
disk I/O and does not respect the page_count. But it is thread-safe.
*/


namespace moderndbs {

BufferFrame::BufferFrame(BufferFrame&& o) noexcept : exclusive(o.exclusive), data(move(o.data)) {

}

BufferFrame& BufferFrame::operator=(BufferFrame&& o) noexcept {
   exclusive = o.exclusive;
   data = move(o.data);
   return *this;
};


char* BufferFrame::get_data() {
   return data.data();
}


BufferManager::BufferManager(size_t page_size, size_t /*page_count*/) : page_size(page_size) {
}


BufferManager::~BufferManager() = default;


BufferFrame& BufferManager::fix_page(uint64_t page_id, bool exclusive) {
   auto guard = std::unique_lock(bm_lock);
   auto result = pages.emplace(page_id, BufferFrame{});
   auto& page = result.first->second;
   bool is_new = result.second;
   if (is_new) {
      page.data.resize(page_size, 0);
   }

   // Ensure that the buffer manager does not deadlock.
   // This is only safe, because we never evict pages!
   guard.unlock();
   if (exclusive) {
      page.latch.lock();
      page.exclusive = true;
   } else {
      page.latch.lock_shared();
      assert(!page.exclusive);
   }

   return page;
}


void BufferManager::unfix_page(BufferFrame& page, bool /*is_dirty*/) {
   if (page.exclusive) {
      page.exclusive = false;
      page.latch.unlock();
   } else {
      page.latch.unlock_shared();
   }
}

}  // namespace moderndbs
