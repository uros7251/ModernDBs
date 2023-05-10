#include "moderndbs/buffer_manager.h"
#include <iostream>
#include <thread>


namespace moderndbs {

BufferManager::BufferManager(size_t page_size, size_t page_count)
   : _page_size(page_size), _page_count(page_count) {
    // TODO: add your implementation here
    _page_io = PageIO::get_instance();
    _buffer = new char[_page_size*_page_count];
    _map.reserve(page_count);
}

BufferManager::~BufferManager() {
    // TODO: add your implementation here
    for (auto& frame: _fifo) {
        flush_frame(frame);
    }
    for (auto& frame: _lru) {
        flush_frame(frame);
    }
    delete[] _buffer;
}

BufferFrame& BufferManager::fix_page(uint64_t page_id, bool exclusive) {
    // TODO: add your implementation here
    std::unique_lock lck {mtx};
    if (_map.find(page_id) == _map.end()) {
        // if the page is not in the buffer
        if (_map.size() < _page_count) {
            // if there's free space inside the buffer
            // init new BufferFrame object
            BufferFrame frame(page_id, exclusive, &_buffer[_map.size() * _page_size] );
            // insert into fifo queue and hash map
            _fifo.push_back(frame);
            auto iter = std::prev(_fifo.end());
            _map.insert({page_id, {true, iter}});
            // release the buffer manager mutex
            lck.unlock();
            // read data from persistent storage
            read_page(*iter);
            // signal that data is ready to be consumed
            {
                std::unique_lock lk {iter->cv_m};
                iter->ready = true;
            }
            iter->cv.notify_all();
            // lock the buffer frame mutex
            if (exclusive)  iter->latch.lock();
            else iter->latch.lock_shared();
            // return BufferFrame reference
            return *iter;
        }
        else {
            // no free space in the buffer => need to replace some page
            // check fifo first
            auto iter = _fifo.begin();
            while (iter != _fifo.end() && iter->pin_count > 0) ++iter;
            if (iter != _fifo.end()) {
                /*
                // to-be-replaced page is in fifo queue
                BufferFrame replaced_page(*iter);
                // remove from map
                _map.erase(iter->page_id);
                // update structure
                iter->page_id = page_id;
                iter->pin_count = 1;
                iter->ready = false;
                // move it to the back of the fifo queue
                _fifo.splice(_fifo.end(), _fifo, iter);
                // insert into hash map
                _map[page_id] = IterWrapper(true, iter);
                // release the lock for write-back
                lck.unlock();
                // flush to persistent storage if needed
                flush_frame(replaced_page);
                // read new page to memory
                read_page(*iter);
                // signal that data is ready to be consumed
                {
                    std::unique_lock lk {iter->cv_m};
                    iter->ready = true;
                }
                iter->cv.notify_all();
                // obtain a lock
                if (exclusive) iter->latch.lock();
                else iter->latch.lock_shared();
                // modify structure
                iter->dirty = false;
                iter->exclusive = exclusive; */
                replace_page(lck, iter, _fifo, page_id, exclusive);
                // return
                return *iter;
            }
            else {
                // ..and only then lru
                iter = _lru.begin();
                while (iter != _lru.end() && iter->pin_count > 0) ++iter;
                if (iter != _lru.end()) {
                    /*
                    // to-be-replaced page is in fifo queue
                    BufferFrame replaced_page(*iter);
                    // remove from map
                    _map.erase(iter->page_id);
                    // update structure
                    iter->page_id = page_id;
                    iter->pin_count = 1;
                    iter->ready = false;
                    // move it to the back of the fifo queue
                    _fifo.splice(_fifo.end(), _lru, iter);
                    // insert into hash map
                    _map[page_id] = IterWrapper(true, iter);
                    // release the lock for write-back
                    lck.unlock();
                    // flush to persistent storage if needed
                    flush_frame(replaced_page);
                    // read new page to memory
                    read_page(*iter);
                    // signal that data is ready to be consumed
                    {
                        std::unique_lock lk {iter->cv_m};
                        iter->ready = true;
                    }
                    iter->cv.notify_all();
                    // obtain a lock
                    if (exclusive) iter->latch.lock();
                    else iter->latch.lock_shared();
                    // modify structure
                    iter->dirty = false;
                    iter->exclusive = exclusive;*/
                    replace_page(lck, iter, _lru, page_id, exclusive);
                    // return
                    return *iter;
                }
                else {
                    // all pages are fixed and non can be replaced
                    // release the buffer manager mutex
                    lck.unlock();
                    throw buffer_full_error{};
                }
            }
        }
    }
    else {
        // page is in the buffer
        // move to the back of lru queue
        auto& iter_wrapper = _map[page_id];
        auto& iter = iter_wrapper.it;
        if (iter_wrapper.in_fifo) {
            _lru.splice(_lru.end(), _fifo, iter);
            iter_wrapper.in_fifo = false;
        }
        else {
            _lru.splice(_lru.end(), _lru, iter);
        }
        // update pin_count
        ++(iter->pin_count);
         // release the buffer manager mutex
        lck.unlock();
        {
            std::unique_lock lk {iter->cv_m};
            iter->cv.wait(lk, [&iter]{return iter->ready;});
        }
        // lock appropriately
        if (exclusive) {
            iter->latch.lock();
            iter->exclusive = true;
        }
        else {
            iter->latch.lock_shared();
        }
        // return reference
        return * iter;
    }
}

void BufferManager::unfix_page(BufferFrame& page, bool is_dirty) {
    if (page.exclusive == true) {
        page.exclusive = false;
        page.latch.unlock();
    }
    else {
        page.latch.unlock_shared();
    }
    {
        std::unique_lock lck {mtx};
        auto& iter_wrapper = _map[page.page_id];
        if (!iter_wrapper.in_fifo) {
            _lru.splice(_lru.end(), _lru, iter_wrapper.it);
            //_fifo.splice(_fifo.end(), _fifo, iter_wrapper.it);
        }
        // update buffer frame fields
        page.dirty = is_dirty;
        --page.pin_count;
    }
}

std::vector<uint64_t> BufferManager::get_fifo_list() const {
    // TODO: add your implementation here
    std::vector<uint64_t> result;
    for (auto& frame: _fifo) result.push_back(frame.page_id);
    return result;
}

std::vector<uint64_t> BufferManager::get_lru_list() const {
    // TODO: add your implementation here
    std::vector<uint64_t> result;
    for (auto& frame: _lru) result.push_back(frame.page_id);
    return result;
}

inline void moderndbs::BufferManager::flush_frame(BufferFrame& frame) {
    // only reading dirty, no harm
    if (!frame.dirty) return;
    _page_io->write_page(frame.page_id, frame.data, _page_size);
}

inline void BufferManager::read_page(BufferFrame& frame) {
    _page_io->read_page(frame.page_id, frame.data, _page_size);
}

void BufferManager::replace_page(std::unique_lock<std::mutex>& lck, std::list<BufferFrame>::iterator& iter, std::list<BufferFrame>& source, uint64_t page_id, bool exclusive) {
    // to-be-replaced page is in fifo queue
    BufferFrame replaced_page(*iter);
    // remove from map
    _map.erase(iter->page_id);
    // update structure
    iter->page_id = page_id;
    iter->pin_count = 1;
    iter->ready = false;
    // move it to the back of the fifo queue
    _fifo.splice(_fifo.end(), source, iter);
    // insert into hash map
    _map.insert({page_id, {true, iter}});
    // release the lock for write-back
    lck.unlock();
    // flush to persistent storage if needed
    flush_frame(replaced_page);
    // read new page to memory
    read_page(*iter);
    // signal that data is ready to be consumed
    {
        std::unique_lock lk {iter->cv_m};
        iter->ready = true;
    }
    iter->cv.notify_all();
    // obtain a lock
    if (exclusive) iter->latch.lock();
    else iter->latch.lock_shared();
    // modify structure
    iter->dirty = false;
    iter->exclusive = exclusive;
}

bool PageIO::read_page(uint64_t page_id, char* buffer, size_t size) {
    mtx.lock();
    auto segment_id = BufferManager::get_segment_id(page_id);
    if (segment_file_handle.find(segment_id) == segment_file_handle.end() ||
        offset.find(page_id) == offset.end()) {
        mtx.unlock();
        return false;
    }
    auto& file = *segment_file_handle[segment_id];
    if (file.size() < offset[page_id]+size) {
        mtx.unlock();
        return false;
    }
    mtx.unlock();
    file.read_block(
        offset[page_id],
        size,
        buffer);
    return true;
}

void PageIO::write_page(uint64_t page_id, char* buffer, size_t size) {
    mtx.lock();
    auto segment_id = BufferManager::get_segment_id(page_id);
    if (segment_file_handle.find(segment_id) == segment_file_handle.end()) {
        auto filename = "file." + std::to_string(segment_id);
        segment_file_handle[segment_id] = File::open_file(filename.data(), File::Mode::WRITE);
        last_offset[segment_id] = segment_file_handle[segment_id]->size();
    }
    auto& file = *segment_file_handle[segment_id];
    if (offset.find(page_id) == offset.end()) {
        offset[page_id] = last_offset[segment_id];
        last_offset[segment_id] += size;
    }
    if (file.size() < offset[page_id] + size) {
        file.resize(2*(offset[page_id]+size));
    }
    mtx.unlock();
    file.write_block(
        buffer,
        offset[page_id],
        size);
}

std::shared_ptr<PageIO> PageIO::get_instance() {
    static std::shared_ptr<PageIO> instance(new PageIO());
    return instance;
}
} // namespace moderndbs
