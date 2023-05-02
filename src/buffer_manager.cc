#include "moderndbs/buffer_manager.h"


namespace moderndbs {

char* BufferFrame::get_data() {
    // TODO: add your implementation here
    return nullptr;
}


BufferManager::BufferManager(size_t page_size, size_t page_count) {
    // TODO: add your implementation here
}


BufferManager::~BufferManager() {
    // TODO: add your implementation here
}


BufferFrame& BufferManager::fix_page(uint64_t page_id, bool exclusive) {
    // TODO: add your implementation here
    throw buffer_full_error{};
}


void BufferManager::unfix_page(BufferFrame& page, bool is_dirty) {
    // TODO: add your implementation here
}


std::vector<uint64_t> BufferManager::get_fifo_list() const {
    // TODO: add your implementation here
    return {};
}


std::vector<uint64_t> BufferManager::get_lru_list() const {
    // TODO: add your implementation here
    return {};
}

}  // namespace moderndbs
