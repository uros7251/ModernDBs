#include "moderndbs/slotted_page.h"
#include <cstring>

using moderndbs::SlottedPage;

SlottedPage::Header::Header(uint32_t page_size)
   : slot_count(0),
     first_free_slot(0),
     data_start(page_size),
     free_space(page_size - sizeof(Header)) {}

SlottedPage::SlottedPage(uint32_t page_size)
   : header(page_size) {
   std::memset(get_data() + sizeof(SlottedPage), 0x00, page_size - sizeof(SlottedPage));
}

std::byte *SlottedPage::get_data() {
   return reinterpret_cast<std::byte*>(this);
}

const std::byte *SlottedPage::get_data() const {
   return reinterpret_cast<const std::byte*>(this);
}

SlottedPage::Slot *SlottedPage::get_slots() {
   return reinterpret_cast<SlottedPage::Slot*>(get_data() + sizeof(SlottedPage));
}

const SlottedPage::Slot *SlottedPage::get_slots() const {
   return reinterpret_cast<const SlottedPage::Slot*>(get_data() + sizeof(SlottedPage));
}

uint32_t SlottedPage::get_fragmented_free_space() {
   // TODO: add your implementation here
   throw std::logic_error{"not implemented"};
}

uint16_t SlottedPage::allocate(uint32_t data_size, uint32_t page_size) {
   // TODO: add your implementation here
   throw std::logic_error{"not implemented"};
}

void SlottedPage::relocate(uint16_t slot_id, uint32_t data_size, uint32_t page_size) {
   // TODO: add your implementation here
   throw std::logic_error{"not implemented"};
}

void SlottedPage::erase(uint16_t slot_id) {
   // TODO: add your implementation here
   throw std::logic_error{"not implemented"};
}

void SlottedPage::compactify(uint32_t page_size) {
   // TODO: add your implementation here
   throw std::logic_error{"not implemented"};
}
